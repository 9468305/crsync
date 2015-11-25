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
#ifndef CRS_HELPER_H
#define CRS_HELPER_H

#include <stdint.h>

#include "define.h"
#include "diff.h"

CRScode helper_perform_version();

typedef struct helper_t {
    //here is constant ref pointer, never change
    char *fileDir; //file directory, end with '/', ref to bulkhelper_t.fileDir
    char *baseUrl; //base url, end with '/', ref to bulkhelper_t.baseUrl

    //here is constant, never change
    char *fileName; //file name
    uint32_t fileSize; //whole file size
    uint8_t fileDigest[CRS_STRONG_DIGEST_SIZE]; //whole file digest

    //here is variable, which may change in diff and patch

    uint32_t cacheSize; //local, cache, same file size
    int isComplete; //0 not; 1 complete
    fileDigest_t *fd;
    diffResult_t *dr;
} helper_t;

helper_t* helper_malloc();
void helper_free(helper_t *h);

CRScode helper_perform_diff(helper_t *h);

CRScode helper_perform_patch(helper_t *h);

typedef struct bulkHelper_t {
    //here is constant, never change
    char *fileDir;
    char *baseUrl;

    //here is bulk file struct
    unsigned int bulkSize; //bulk file Num
    helper_t *bulk;

} bulkHelper_t;

bulkHelper_t * bulkHelper_malloc();
void bulkHelper_free(bulkHelper_t *bh);

CRScode bulkHelper_perform_diff(bulkHelper_t *bh);

CRScode bulkHelper_perform_patch(bulkHelper_t *bh);

#endif // CRS_HELPER_H
