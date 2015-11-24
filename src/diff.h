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
#ifndef CRS_DIFF_H
#define CRS_DIFF_H

#include <stdint.h>

#include "define.h"
#include "digest.h"

typedef struct diffResult_t {
    int32_t totalNum; //should be filedigest_t.fileSize / filedigest_t.blockSize;
    int32_t matchNum; //calc from filedigest_t.offsets, compare to source file
    int32_t cacheNum; //dst file already got
    int32_t *offsets; //performed result, -1(default) miss, -2 cache, >=0 offset at source file;
} diffResult_t;

diffResult_t* diffResult_malloc();
void diffResult_free(diffResult_t *dr);

void diffResult_dump(diffResult_t *dr);

CRScode Diff_perform(const char *filename, const filedigest_t *fd, diffResult_t *dr);


#endif // CRS_DIFF_H

