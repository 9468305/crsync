/*
The MIT License (MIT)

Copyright (c) 2015 chenqi

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <sys/stat.h>
#include <omp.h>

#include "diff.h"
#include "uthash.h"
#include "log.h"

typedef struct diffHash_t {
    uint32_t            weak; //first key
    int32_t             seq; //second key
    uint8_t             *strong; //reference pointer
    struct diffHash_t   *sub; //sub HashTable
    UT_hash_handle      hh;
} diffHash_t;

static diffHash_t* diffHash_malloc() {
    diffHash_t *dh = calloc(1, sizeof(diffHash_t));
    return dh;
}

static void diffHash_free(diffHash_t **dh) {
    diffHash_t *sumItem=NULL, *sumTemp=NULL, *sumIter=NULL, *sumTemp2=NULL;
    HASH_ITER(hh, *dh, sumItem, sumTemp) {
        HASH_ITER(hh, sumItem->sub, sumIter, sumTemp2 ) {
            HASH_DEL(sumItem->sub, sumIter);
            free(sumIter);
        }
        HASH_DEL(*dh, sumItem);
        free(sumItem);
    }
}

diffResult_t* diffResult_malloc() {
    diffResult_t *dr = calloc(1, sizeof(diffResult_t));
    return dr;
}

void diffResult_free(diffResult_t *dr) {
    if(dr) {
        free(dr->offsets);
        free(dr);
    }
}

void diffResult_dump(diffResult_t *dr) {
    if(dr){
        LOGI("totalNum = %d\n", dr->totalNum);
        LOGI("matchNum = %d\n", dr->matchNum);
        LOGI("cacheNum = %d\n", dr->cacheNum);
        LOGI("missNum = %d\n", dr->totalNum - dr->matchNum - dr->cacheNum);
    } else {
        LOGI("none\n");
    }
}

static diffHash_t* Diff_hash(const filedigest_t *fd) {
    diffHash_t *dh = NULL;
    diffHash_t *item = NULL, *temp = NULL;
    uint32_t blockNum = fd->fileSize / fd->blockSize;

    for(size_t i=0; i<blockNum; ++i) {

        item = diffHash_malloc();
        item->weak = fd->blockDigest[i].weak;
        item->seq = i;
        item->strong = fd->blockDigest[i].strong;

        HASH_FIND_INT(dh, &item->weak, temp);
        if (!temp) {
            HASH_ADD_INT( dh, weak, item );
        } else {
            HASH_ADD_INT( temp->sub, seq, item );
        }
    }
    return dh;
}


#define DIFF_PARALLELISM_DEGREE 4

static void Diff_match(const char *filename, const filedigest_t *fd, const diffHash_t **dh, diffResult_t *dr) {
    dr->totalNum = fd->fileSize / fd->blockSize;
    dr->matchNum = 0;
    dr->cacheNum = 0;
    dr->offsets = malloc(dr->totalNum * sizeof(int32_t));
    memset(dr->offsets, -1, dr->totalNum * sizeof(int32_t));

    struct stat st;
    if(stat(filename, &st)!=0 || (size_t)st.st_size <= fd->blockSize*DIFF_PARALLELISM_DEGREE) {
        // file not exist || small file
        return;
    }

    const size_t parallel_size = (st.st_size + DIFF_PARALLELISM_DEGREE - 1) / DIFF_PARALLELISM_DEGREE;

#pragma omp parallel shared(fd, dh, dr), num_threads(DIFF_PARALLELISM_DEGREE)
    {
        size_t id__ = omp_get_thread_num();
        FILE *file = fopen(filename, "rb");
        if(file)
        {
            size_t read_begin = 0;
            size_t read_end = 0;
            if(id__ == 0) {
                read_begin = 0;
                read_end = parallel_size;
            } else if(id__ == DIFF_PARALLELISM_DEGREE-1) {
                read_begin = id__ * parallel_size - fd->blockSize + 1;
                read_end = st.st_size;
            } else {
                read_begin = id__ * parallel_size - fd->blockSize + 1;
                read_end = (id__+1) * parallel_size;
            }
            size_t read_len = read_end - read_begin;
            size_t offset = read_begin;

            unsigned char *buf1 = malloc(fd->blockSize);
            unsigned char *buf2 = malloc(fd->blockSize);

            size_t r;

            r = fseek(file, read_begin, SEEK_SET);
            if(r != 0) {
                LOGE("error fseek\n");
            }
            r = fread(buf1, 1, fd->blockSize, file);
            read_len -= fd->blockSize;
            if(r != fd->blockSize) {
                LOGE("error fread\n");
            }
            uint32_t weak;
            uint8_t strong[CRS_STRONG_DIGEST_SIZE];
            diffHash_t *sumItem = NULL, *sumIter = NULL, *sumTemp = NULL;

            //Digest_match_first
            Digest_CalcWeak_Data(buf1, fd->blockSize, &weak);
            HASH_FIND_INT( *dh, &weak, sumItem );
            if(sumItem) {
                Digest_CalcStrong_Data(buf1, fd->blockSize, strong);
                if (0 == memcmp(strong, sumItem->strong, CRS_STRONG_DIGEST_SIZE)) {
                    dr->offsets[sumItem->seq] = offset;
                }
                HASH_ITER(hh, sumItem->sub, sumIter, sumTemp) {
                    if (0 == memcmp(strong, sumIter->strong, CRS_STRONG_DIGEST_SIZE)) {
                        dr->offsets[sumIter->seq] = offset;
                    }
                }
            }

            //Digest_match_loop
            while(read_len >= fd->blockSize) {
                r = fread(buf2, 1, fd->blockSize, file);
                if(r != fd->blockSize) {
                    LOGE("error fread\n");
                }
                read_len -= fd->blockSize;

                for(size_t i=0; i<fd->blockSize;) {
                    Digest_CalcWeak_Roll(buf1[i], buf2[i], fd->blockSize, &weak);
                    ++i;
                    ++offset;
                    HASH_FIND_INT( *dh, &weak, sumItem );
                    if(sumItem) {
                        Digest_CalcStrong_Data2(buf1, buf2, fd->blockSize, i, strong);
                        if (0 == memcmp(strong, sumItem->strong, CRS_STRONG_DIGEST_SIZE)) {
                            dr->offsets[sumItem->seq] = offset;
                        }
                        HASH_ITER(hh, sumItem->sub, sumIter, sumTemp) {
                            if (0 == memcmp(strong, sumIter->strong, CRS_STRONG_DIGEST_SIZE)) {
                                dr->offsets[sumIter->seq] = offset;
                            }
                        }
                    }
                }
                //switch buffer
                uint8_t *tmpbuf = buf1;
                buf1 = buf2;
                buf2 = tmpbuf;
            }

            //Digest_match_end
            if(read_len > 0) {
                r = fread(buf2, 1, read_len, file);
                if(r != read_len) {
                    LOGE("error fread\n");
                }
                for(size_t i=0; i<read_len;) {
                    Digest_CalcWeak_Roll(buf1[i], buf2[i], fd->blockSize, &weak);
                    ++i;
                    ++offset;
                    HASH_FIND_INT( *dh, &weak, sumItem );
                    if(sumItem) {
                        Digest_CalcStrong_Data2(buf1, buf2, fd->blockSize, i, strong);
                        if (0 == memcmp(strong, sumItem->strong, CRS_STRONG_DIGEST_SIZE)) {
                            dr->offsets[sumItem->seq] = offset;
                        }
                        HASH_ITER(hh, sumItem->sub, sumIter, sumTemp) {
                            if (0 == memcmp(strong, sumIter->strong, CRS_STRONG_DIGEST_SIZE)) {
                                dr->offsets[sumIter->seq] = offset;
                            }
                        }
                    }
                }
            }

            free(buf1);
            free(buf2);
            fclose(file);
        }//end of if(file)
    }//end of omp parallel (DIFF_PARALLELISM_DEGREE)

    for(int32_t i=0; i< dr->totalNum; ++i) {
        if(dr->offsets[i] >= 0) {
            dr->matchNum++;
        }
    }
}

CRScode Diff_perform(const char *filename, const filedigest_t *fd, diffResult_t *dr) {
    LOGI("begin\n");
    if(!filename || !fd || !dr) {
        LOGE("end %d\n", CRS_PARAM_ERROR);
        return CRS_PARAM_ERROR;
    }

    CRScode code = CRS_OK;
    diffHash_t *dh = Diff_hash(fd);

    Diff_match(filename, fd, (const diffHash_t **)&dh, dr);

    diffHash_free(&dh);

    LOGI("end %d\n", code);
    return code;
}
