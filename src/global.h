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
#ifndef CRS_GLOBAL_H
#define CRS_GLOBAL_H

#if defined __cplusplus
extern "C" {
#endif

typedef enum {
    CRS_OK = 0,
    CRS_INIT_ERROR, //mostly curl init error
    CRS_PARAM_ERROR, //input parameters wrong
    CRS_FILE_ERROR, //file io error
    CRS_HTTP_ERROR, //http io error
    CRS_USER_CANCEL, //user cancel (manual operation, Network Policy)

    CRS_BUG = 100, //Internal error
} CRScode;

#define CRS_STRONG_DIGEST_SIZE 16

extern void crs_callback_diff   (const char *basename, const unsigned int bytes, const int isComplete, const int isNew);
extern int  crs_callback_patch  (const char *basename, const unsigned int bytes, const int isComplete, const int immediate);

#if defined __cplusplus
}
#endif

#endif // CRS_GLOBAL_H
