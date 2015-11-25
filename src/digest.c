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
#include <stdlib.h>
#include <sys/stat.h>

#include "digest.h"
#include "blake2.h"
#include "md5.h"
#include "log.h"
#include "util.h"
#include "tpl.h"

void Digest_CalcWeak_Data(const uint8_t *data, const uint32_t size, uint32_t *out) {
    uint32_t i = 0, a = 0, b = 0;
    const uint8_t *p = data;
    for ( ; i < size - 4; i += 4) {
        b += 4 * (a + p[i]) + 3 * p[i + 1] + 2 * p[i + 2] + p[i + 3];
        a += p[i + 0] + p[i + 1] + p[i + 2] + p[i + 3];
    }
    for ( ; i < size; i++) {
        a += p[i];
        b += a;
    }
    *out = (a & 0xffff) | (b << 16);
}

void Digest_CalcWeak_Roll(const uint8_t out, const uint8_t in, const uint32_t blockSize, uint32_t *weak) {
    uint32_t a = *weak & 0xffff;
    uint32_t b = *weak >> 16;

    a += in - out;
    b += a - blockSize * out;

    *weak = (a & 0xffff) | (b << 16);
}

void Digest_CalcStrong_Data(const uint8_t *data, const uint32_t size, uint8_t *out) {
    blake2b_state ctx;
    blake2b_init(&ctx, CRS_STRONG_DIGEST_SIZE);
    blake2b_update(&ctx, data, size);
    blake2b_final(&ctx, (uint8_t *)out, CRS_STRONG_DIGEST_SIZE);
}

void Digest_CalcStrong_Data2(const uint8_t *buf1, const uint8_t *buf2, const uint32_t size, const uint32_t offset, uint8_t *out) {
    blake2b_state ctx;
    blake2b_init(&ctx, CRS_STRONG_DIGEST_SIZE);
    blake2b_update(&ctx, buf1+offset, size-offset);
    blake2b_update(&ctx, buf2, offset);
    blake2b_final(&ctx, (uint8_t *)out, CRS_STRONG_DIGEST_SIZE);
}

int Digest_CalcStrong_File(const char *filename, uint8_t *out) {
    return blake2b_File(filename, out, CRS_STRONG_DIGEST_SIZE);
}

fileDigest_t* fileDigest_malloc() {
    return calloc(1, sizeof(fileDigest_t));
}

void fileDigest_free(fileDigest_t* fd) {
    if(fd) {
        free(fd->blockDigest);
        free(fd->restData);
        free(fd);
    }
}

void fileDigest_dump(const fileDigest_t* fd) {
    if(fd) {
        LOGI("fileSize = %d\n", fd->fileSize);
        LOGI("blockSize = %d KiB\n", fd->blockSize/1024);
        char *hashString = Util_hex_string(fd->fileDigest, CRS_STRONG_DIGEST_SIZE);
        LOGI("fileDigest = %s\n", hashString);
        free(hashString);
        size_t restSize = (fd->blockSize == 0) ? 0 : (fd->fileSize % fd->blockSize);
        LOGI("restSize = %d Bytes\n", restSize);
    } else {
        LOGI("none\n");
    }
}

CRScode Digest_Perform(const char *filename, const uint32_t blockSize, fileDigest_t *fd) {
    LOGI("begin\n");

    if(!filename || blockSize == 0 || !fd) {
        LOGE("end %d\n", CRS_PARAM_ERROR);
        return CRS_PARAM_ERROR;
    }

    struct stat st;
    if(stat(filename, &st)!=0 || st.st_size==0){
        // file not exist || file size is zero
        LOGE("end %s stat\n", filename);
        return CRS_FILE_ERROR;
    }

    FILE *f = fopen(filename, "rb");
    if(!f) {
        LOGE("end %s fopen\n", filename);
        return CRS_FILE_ERROR;
    }

    CRScode code = CRS_OK;
    uint8_t *buf = malloc(blockSize);
    uint32_t blockNum = st.st_size / blockSize;
    uint32_t restSize = st.st_size % blockSize;
    uint8_t *restData = (restSize > 0) ? malloc(restSize) : NULL;
    digest_t *digests = (blockNum > 0) ? malloc(sizeof(digest_t) * blockNum) : NULL;

    for(uint32_t i=0; i< blockNum; ++i) {
        if(fread(buf, 1, blockSize, f) == blockSize) {
            Digest_CalcWeak_Data(buf, blockSize, &digests[i].weak);
            Digest_CalcStrong_Data(buf, blockSize, digests[i].strong);
        } else {
            code = CRS_FILE_ERROR;
            LOGE("error %s fread\n", filename);
            break;
        }
    }

    if(code == CRS_OK && restSize > 0) {
        if(fread(buf, 1, restSize, f) == restSize) {
            memcpy(restData, buf, restSize);
        } else {
            code = CRS_FILE_ERROR;
            LOGE("error %s fread\n", filename);
        }
    }
    free(buf);
    fclose(f);

    if(code != CRS_OK) {
        LOGE("end %d\n", code);
        return code;
    }

    uint8_t fileDigest[CRS_STRONG_DIGEST_SIZE];
    if(0 != Digest_CalcStrong_File(filename, fileDigest)) {
        LOGE("end %s Digest_CalcStrong_File\n", filename);
        return CRS_FILE_ERROR;
    }

    fd->fileSize = st.st_size;
    fd->blockSize = blockSize;
    fd->blockDigest = digests;
    fd->restData = restData;
    memcpy(fd->fileDigest, fileDigest, CRS_STRONG_DIGEST_SIZE);

    LOGI("end %d\n", code);
    return code;
}

static const char *DIGEST_TPLMAP_FORMAT = "uuc#BA(uc#)";

CRScode Digest_Load(const char *filename, fileDigest_t *fd) {
    LOGI("begin\n");

    if(!filename || !fd) {
        LOGE("end %d\n", CRS_PARAM_ERROR);
        return CRS_PARAM_ERROR;
    }

    if(0 != Digest_checkfile(filename)) {
        LOGI("end %s miss\n", filename);
        return CRS_FILE_ERROR;
    }

    CRScode code = CRS_OK;
    tpl_bin tb = {NULL, 0};
    digest_t digest;

    tpl_node *tn = tpl_map( DIGEST_TPLMAP_FORMAT,
                            &fd->fileSize,
                            &fd->blockSize,
                            fd->fileDigest,
                            CRS_STRONG_DIGEST_SIZE,
                            &tb,
                            &digest.weak,
                            &digest.strong,
                            CRS_STRONG_DIGEST_SIZE);
    if(0 == tpl_load(tn, TPL_FILE, filename)) {
        tpl_unpack(tn, 0);

        uint32_t blockNum = fd->fileSize / fd->blockSize;
        fd->blockDigest = (blockNum > 0) ? malloc(sizeof(digest_t) * blockNum) : NULL;
        fd->restData = tb.addr;

        for (uint32_t i = 0; i < blockNum; i++) {
            tpl_unpack(tn, 1);
            fd->blockDigest[i].weak = digest.weak;
            memcpy(fd->blockDigest[i].strong, digest.strong, CRS_STRONG_DIGEST_SIZE);
        }
    } else {
        LOGE("error tpl_load %s\n", filename);
        code = CRS_FILE_ERROR;
    }
    tpl_free(tn);

    LOGI("end %d\n", code);
    return code;
}

CRScode Digest_Save(const char *filename, fileDigest_t *fd) {
    LOGI("begin\n");

    if(!filename || !fd) {
        LOGE("end %d\n", CRS_PARAM_ERROR);
        return CRS_PARAM_ERROR;
    }

    CRScode code = CRS_OK;
    tpl_bin tb = {NULL, 0};
    tb.addr = fd->restData;
    tb.sz = fd->fileSize % fd->blockSize;

    digest_t digest;

    tpl_node *tn = tpl_map( DIGEST_TPLMAP_FORMAT,
                            &fd->fileSize,
                            &fd->blockSize,
                            fd->fileDigest,
                            CRS_STRONG_DIGEST_SIZE,
                            &tb,
                            &digest.weak,
                            &digest.strong,
                            CRS_STRONG_DIGEST_SIZE);
    tpl_pack(tn, 0);

    uint32_t blockNum = fd->fileSize / fd->blockSize;
    for (uint32_t i = 0; i < blockNum; ++i) {
        digest.weak = fd->blockDigest[i].weak;
        memcpy(digest.strong, fd->blockDigest[i].strong, CRS_STRONG_DIGEST_SIZE);
        tpl_pack(tn, 1);
    }

    if(0 != tpl_dump(tn, TPL_FILE, filename)) {
        LOGE("error tpl_dump %s\n", filename);
        code = CRS_FILE_ERROR;
    }
    tpl_free(tn);

    LOGI("end %d\n", code);
    return code;
}

int Digest_checkfile(const char *filename) {
    return Util_tplfile_check(filename, DIGEST_TPLMAP_FORMAT);
}
