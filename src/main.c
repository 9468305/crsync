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
#include "curl/curl.h"

#if defined __cplusplus
extern "C" {
#endif

#define LOGI(...)  printf(__VA_ARGS__)

int main(void)
{
    const char *newFilename = "mthd.apk";
    const char *oldFilename = "mtsd.apk";
    const char *rsumsURL = "http://image.ib.qq.com/a/test/mthd.apk.rsums";
    const char *newFileURL = "http://image.ib.qq.com/a/test/mthd.apk";

    printf("crsync main begin\n");
    //crsync_server("mthd.apk", "./");
    crsync_client(oldFilename, newFilename, rsumsURL, newFileURL);

    printf("crsync main end\n");
    return 0;
}

#if defined __cplusplus
    }
#endif
