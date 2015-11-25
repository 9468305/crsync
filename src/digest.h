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
#ifndef CRS_DIGEST_H
#define CRS_DIGEST_H

#include <stdint.h>

#include "define.h"

#define DIGEST_EXT ".rsum"

void Digest_CalcWeak_Data(const uint8_t *data, const uint32_t len, uint32_t *out);
void Digest_CalcWeak_Roll(const uint8_t out, const uint8_t in, const uint32_t blockSize, uint32_t *weak);

void Digest_CalcStrong_Data(const uint8_t *data, const uint32_t len, uint8_t *out);
void Digest_CalcStrong_Data2(const uint8_t *buf1, const uint8_t *buf2, const uint32_t size, const uint32_t offset, uint8_t *out);
int  Digest_CalcStrong_File(const char *filename, uint8_t *out);

typedef struct digest_t {
    uint8_t     strong[CRS_STRONG_DIGEST_SIZE]; // strong digest (md5, blake2 etc.)
    uint32_t    weak; // Adler32, used for Rolling calc
} digest_t;

typedef struct fileDigest_t {
    uint32_t    fileSize; //file size
    uint32_t    blockSize; //block size
    uint8_t     fileDigest[CRS_STRONG_DIGEST_SIZE]; //file strong sum
    digest_t    *blockDigest; //every block's rsum_t data
    uint8_t     *restData; //rest binary data, size = fileSize % blockSize
} fileDigest_t;

fileDigest_t* fileDigest_malloc();
void          fileDigest_free(fileDigest_t* fd);
void          fileDigest_dump(const fileDigest_t* fd);

CRScode Digest_Perform(const char *filename, const uint32_t blockSize, fileDigest_t *fd);
CRScode Digest_Load(const char *filename, fileDigest_t *fd);
CRScode Digest_Save(const char *filename, fileDigest_t *fd);
int     Digest_checkfile(const char *filename);

#endif // CRS_DIGEST_H
