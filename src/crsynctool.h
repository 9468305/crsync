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
#ifndef CRSYNC_TOOL_H
#define CRSYNC_TOOL_H

#include "crsync.h"

#if defined __cplusplus
extern "C" {
#endif

typedef struct crsynctool_handle_t crsynctool_handle_t;

typedef enum {
    CRSYNCTOOLOPT_ROOT = 1,     /* local root dir */
    CRSYNCTOOLOPT_FILE,         /* local file name */
    CRSYNCTOOLOPT_BLOCKSIZE,    /* block size, default 2K */
} CRSYNCTOOLoption;

/* return: NULL for fail */
crsynctool_handle_t* crsynctool_easy_init();

/* return: only CRSYNCE_OK or CRSYNC_INVALID_OPT */
CRSYNCcode crsynctool_easy_setopt(crsynctool_handle_t *handle, CRSYNCTOOLoption opt, ...);

/* return CRSYNCcode (<=0) or CURLcode (>=0) */
CRSYNCcode crsynctool_easy_perform(crsynctool_handle_t *handle);

void crsynctool_easy_cleanup(crsynctool_handle_t *handle);

#if defined __cplusplus
    }
#endif

#endif // CRSYNC_TOOL_H
