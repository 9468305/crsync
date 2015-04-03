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
#include <stdio.h>
#include <time.h>

#include "crsync.h"

#if defined __cplusplus
extern "C" {
#endif

int main(void)
{
    const char* newFilename = "base14012.obb";
    const char* sigFilename = "base14012.obb.sig";
    const char* oldFilename = "base14009.obb";
    const char* deltaFilename = "base14009.obb.delta";

    clock_t t;

    printf("crsync_hatch begin\n");
    t = clock();
    crsync_hatch(newFilename, sigFilename);
    t -= clock();
    printf("crsync_hatch end %ld\n", -t);

    printf("crsync_match begin\n");
    t = clock();
    crsync_match(oldFilename, sigFilename, deltaFilename);
    t -= clock();
    printf("crsync_match end %ld\n", -t);

    printf("crsync_patch begin\n");
    t = clock();
    crsync_patch(oldFilename, deltaFilename, newFilename);
    t -= clock();
    printf("crsync_patch end %ld\n", -t);

    return 0;
}

#if defined __cplusplus
    }
#endif
