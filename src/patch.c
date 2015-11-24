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
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <omp.h>


#include "patch.h"
#include "util.h"
#include "log.h"
#include "http.h"

static CRScode Patch_match(const char *srcFilename, const char *dstFilename,
                           const filedigest_t *fd, const diffResult_t *dr) {
    LOGI("begin\n");
    if(!srcFilename || !dstFilename || !fd || !dr) {
        LOGE("end %d\n", CRS_PARAM_ERROR);
        return CRS_PARAM_ERROR;
    }

    FILE *f1 = fopen(srcFilename, "rb");
    if(!f1){
        LOGE("source file fopen error %s\n", strerror(errno));
        return CRS_FILE_ERROR;
    }

    FILE *f2 = fopen(dstFilename, "rb+");
    if(!f2){
        LOGE("dest file fopen error %s\n", strerror(errno));
        fclose(f1);
        return CRS_FILE_ERROR;
    }

    CRScode code = CRS_OK;
    uint8_t *buf = malloc(fd->blockSize);

    for(int i=0; i<dr->totalNum; ++i) {
        if(dr->offsets[i] >= 0) {
            fseek(f1, dr->offsets[i], SEEK_SET);
            fread(buf, 1, fd->blockSize, f1);

            fseek(f2, i*fd->blockSize, SEEK_SET);
            fwrite(buf, 1, fd->blockSize, f2);
        }
    }

    size_t restSize = fd->fileSize % fd->blockSize;
    if(restSize > 0){

        fseek(f2, -restSize, SEEK_END);
        fwrite(fd->restData, 1, restSize, f2);
    }

    free(buf);
    fclose(f1);
    fclose(f2);
    LOGI("end %d\n", code);
    return code;
}

static CRScode Patch_cache(const char *filename, const filedigest_t *fd, diffResult_t *dr) {
    if(!filename || !fd || !dr) {
        LOGE("end %d\n", CRS_PARAM_ERROR);
        return CRS_PARAM_ERROR;
    }
    FILE *f = fopen(filename, "rb+");
    if(!f){
        LOGE("dest file fopen error %s\n", strerror(errno));
        return CRS_FILE_ERROR;
    }
    CRScode code = CRS_OK;
    uint8_t *buf = malloc(fd->blockSize);
    uint8_t hash[CRS_STRONG_DIGEST_SIZE];

    for(int i=0; i<dr->totalNum; ++i) {
        if(dr->offsets[i] == -1) {
            fseek(f, i*fd->blockSize, SEEK_SET);
            fread(buf, 1, fd->blockSize, f);

            Digest_CalcStrong_Data(buf, fd->blockSize, hash);
            if(0 == memcmp(hash, fd->blockDigest[i].strong, CRS_STRONG_DIGEST_SIZE)) {
                dr->offsets[i] = -2;
                dr->cacheNum++;
            }
        }
    }
    free(buf);
    diffResult_dump(dr);

    fclose(f);
    return code;
}

//continuous blocks, used for reduce http range frequency
typedef struct combineblock_t {
    size_t pos; //block start position
    size_t got; //data fwrite size, used for HTTP retry
    size_t len; //block length
} combineblock_t;

static uint32_t Patch_missCombine(const diffResult_t *dr, combineblock_t *cb, uint32_t blockSize) {
    uint32_t combineNum = 0;
    int missNum = dr->totalNum - dr->matchNum - dr->cacheNum;
    if(missNum == 0) {
        LOGW("missNum is Zero, maybe cache done!\n");
        return combineNum;
    }
    if(missNum < 0) {
        LOGE("WTF: matchNum %d bigger than totalNum %d\n", dr->matchNum, dr->totalNum);
        return combineNum;
    }
    int i;
    uint32_t j;
    for(i=0; i<dr->totalNum; ++i) {
        if(dr->offsets[i] == -1) {
            for(j=0; j < combineNum; ++j) {
                if(cb[j].pos + cb[j].len == i * blockSize) {
                    cb[j].len += blockSize;
                    break;
                }
            }
            if(j == combineNum) {
                cb[combineNum].pos = i*blockSize;
                cb[combineNum].len = blockSize;
                ++combineNum;
            }
        }
    }
    LOGI("combineblocks Num = %d\n", combineNum);
    return combineNum;
}

typedef struct rangedata_t {
    combineblock_t *cb;
    FILE *file;
} rangedata_t;

static size_t Range_callback(void *data, size_t size, size_t nmemb, void *userp) {
    rangedata_t *rd = (rangedata_t*)userp;
    size_t realSize = size * nmemb;
#pragma omp critical (fileio)
{
    fseek(rd->file, rd->cb->pos + rd->cb->got, SEEK_SET);
    fwrite(data, size, nmemb, rd->file);
    rd->cb->got += realSize;
}
    //TODO callback to user for isCancel
    return realSize;
}

static CRScode Patch_miss(const char *filename, const char *url,
                          const filedigest_t *fd, const diffResult_t *dr) {
    LOGI("begin\n");
    if(!filename || !fd || !dr) {
        LOGE("end %d\n", CRS_PARAM_ERROR);
        return CRS_PARAM_ERROR;
    }

    int missNum = dr->totalNum - dr->matchNum - dr->cacheNum;
    if(missNum == 0) {
        LOGI("end missblocks = 0\n");
        return CRS_OK;
    }

    FILE *f = fopen(filename, "rb+");
    if(!f){
        LOGE("end fopen error %s\n", strerror(errno));
        LOGE("%s\n", filename);
        return CRS_FILE_ERROR;
    }

    //combine continuous blocks, no more than missNum
    combineblock_t *cb = calloc(1, sizeof(combineblock_t) * missNum );
    uint32_t cbNum = Patch_missCombine(dr, cb, fd->blockSize);

    CRScode code = CRS_OK;
#pragma omp parallel for
    for(uint32_t i=0; i< cbNum; ++i) {
        rangedata_t rd;
        rd.cb = &cb[i];
        rd.file = f;
        int retry = 3;
        char range[32];
        long rangeFrom;
        long rangeTo;
        while(retry-- > 0) {
            rangeFrom = cb[i].pos + cb[i].got;
            rangeTo = cb[i].pos + cb[i].len - 1;
            snprintf(range, 32, "%ld-%ld", rangeFrom, rangeTo);
            CRScode reqCode = HTTP_Range(url, range, (void*)Range_callback, &rd);
            if(reqCode == CRS_OK) {
                break;
            }
            /* TODO
            if(user cancel) break;
            */
        }
    }//end of (omp parallel for)

    free(cb);
    fclose(f);
    LOGI("end %d\n", code);
    return code;
}

CRScode Patch_perform(const char *srcFilename, const char *dstFilename, const char *url,
                      const filedigest_t *fd, diffResult_t *dr) {
    LOGI("begin\n");

    if(!srcFilename || !dstFilename || !url || !fd || !dr) {
        LOGE("end %d\n", CRS_PARAM_ERROR);
        return CRS_PARAM_ERROR;
    }

    CRScode code = CRS_OK;
    do {
        if(0 != access(srcFilename, F_OK)) {
            LOGE("src file not exist %s\n", strerror(errno));
            LOGE("%s\n", srcFilename);
            code = CRS_FILE_ERROR;
            break;
        }
        if(0 != access(dstFilename, F_OK)) {
            FILE *f = fopen(dstFilename, "ab+");
            if(f) {
                fclose(f);
            } else {
                LOGE("dst file create fail %s\n", strerror(errno));
                LOGE("%s\n", dstFilename);
                code = CRS_FILE_ERROR;
                break;
            }
        }
        struct stat st;
        if(stat(dstFilename, &st) != 0) {
            LOGE("dst file stat fail %s\n", strerror(errno));
            LOGE("%s\n", dstFilename);
            code = CRS_FILE_ERROR;
            break;
        }
        if((size_t)st.st_size != fd->fileSize) {
            if(0 != truncate(dstFilename, fd->fileSize)) {
                LOGE("dest file truncate %dBytes error %s\n", fd->fileSize, strerror(errno));
                code = CRS_FILE_ERROR;
                break;
            }
        }

        //Patch_match Blocks
        code = Patch_match(srcFilename, dstFilename, fd, dr);
        if(code != CRS_OK) break;

        code = Patch_cache(dstFilename, fd, dr);
        if(code != CRS_OK) break;

        //Patch_miss Blocks
        code = Patch_miss(dstFilename, url, fd, dr);
        if(code != CRS_OK) break;

        uint8_t hash[CRS_STRONG_DIGEST_SIZE];
        Digest_CalcStrong_File(dstFilename, hash);
        char * hashString = Util_HexToString(hash, CRS_STRONG_DIGEST_SIZE);
        LOGI("fileDigest = %s\n", hashString);
        free(hashString);
        code = (0 == memcmp(hash, fd->fileDigest, CRS_STRONG_DIGEST_SIZE)) ? CRS_OK : CRS_BUG ;

    } while (0);

    LOGI("end %d\n", code);
    return code;
}
