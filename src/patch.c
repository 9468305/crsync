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

    FILE *f2 = fopen(dstFilename, "wb");
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

static CRScode Patch_miss(const char *filename, const char *url,
                          const filedigest_t *fd, const diffResult_t *dr) {
    LOGI("begin\n");
    if(!filename || !fd || !dr) {
        LOGE("end %d\n", CRS_PARAM_ERROR);
        return CRS_PARAM_ERROR;
    }

    FILE *f = fopen(filename, "rwb");
    if(!f){
        LOGE("fopen error %s\n", strerror(errno));
        LOGE("%s\n", filename);
        return CRS_FILE_ERROR;
    }

    CRScode code = CRS_OK;
#pragma omp parallel for
    for(int i=0; i< dr->totalNum; ++i) {
        if(dr->offsets[i] < 0) {
            long rangeFrom = i * fd->blockSize;
            long rangeTo = rangeFrom + fd->blockSize - 1;
            char range[32];
            snprintf(range, 32, "%ld-%ld", rangeFrom, rangeTo);
            unsigned char *buf = malloc(fd->blockSize);
            unsigned char strong[CRS_STRONG_DIGEST_SIZE];

#pragma omp critical (fileio)
{
            fseek(f, i* fd->blockSize, SEEK_SET);
            fread(buf, 1, fd->blockSize, f);
}
            Digest_CalcStrong_Data(buf, fd->blockSize, strong);
            if(0 != memcmp(strong, fd->blockDigest[i].strong, CRS_STRONG_DIGEST_SIZE)) {
                code = HTTP_Data(url, range, buf, fd->blockSize, 5);
                if(code == CRS_OK) {
#pragma omp critical (fileio)
{
                    fseek(f, i*fd->blockSize, SEEK_SET);
                    fwrite(buf, 1, fd->blockSize, f);
}
                }
            }
            free(buf);
        }
    }

    fclose(f);
    return code;
}

CRScode Patch_perform(const char *srcFilename, const char *dstFilename, const char *url,
                      const filedigest_t *fd, const diffResult_t *dr) {
    LOGI("begin\n");

    if(!srcFilename || !dstFilename || !url || !fd || !dr) {
        LOGE("end %d\n", CRS_PARAM_ERROR);
        return CRS_PARAM_ERROR;
    }

    CRScode code = CRS_OK;
    do {
        if(0 != access(srcFilename, F_OK)) {
            LOGE("source file not exist %s\n", strerror(errno));
            LOGE("%s\n", srcFilename);
            code = CRS_FILE_ERROR;
            break;
        }
        if(0 != access(dstFilename, F_OK)) {
            FILE *f = fopen(dstFilename, "ab+");
            if(f) {
                fclose(f);
            } else {
                LOGE("dest file create fail %s\n", strerror(errno));
                LOGE("%s\n", dstFilename);
                code = CRS_FILE_ERROR;
                break;
            }
        }
        struct stat st;
        if(stat(dstFilename, &st) != 0) {
            LOGE("dest file stat fail %s\n", strerror(errno));
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
        UTIL_IF_BREAK(code != CRS_OK);

        //Patch_miss Blocks
        code = Patch_miss(dstFilename, url, fd, dr);
        UTIL_IF_BREAK(code != CRS_OK);

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
