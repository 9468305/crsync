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

#if defined __cplusplus
extern "C" {
#endif

#include "crsync.h"

typedef enum {
    CRSYNCTOOLOPT_CURRID = 1,   /* current magnet info id */
    CRSYNCTOOLOPT_NEXTID,       /* next magnet info id */
    CRSYNCTOOLOPT_APPNAME,      /* app name */
    CRSYNCTOOLOPT_RESNAME,      /* resource name array */
    CRSYNCTOOLOPT_APPDIR,       /* app directory */
    CRSYNCTOOLOPT_RESDIR,       /* resource directory */
    CRSYNCTOOLOPT_OUTPUT,       /* output directory */
    CRSYNCTOOLOPT_BLOCKSIZE,    /* block size, default 2K */
} CRSYNCTOOLoption;

typedef struct crsynctool_handle_t crsynctool_handle_t;

/* return: NULL for fail */
crsynctool_handle_t* crsynctool_init();

/* return: only CRSYNCE_OK or CRSYNC_INVALID_OPT */
CRSYNCcode crsynctool_setopt(crsynctool_handle_t *handle, CRSYNCTOOLoption opt, ...);

/* return CRSYNCcode (<=0) or CURLcode (>=0) */
CRSYNCcode crsynctool_perform(crsynctool_handle_t *handle);

void crsynctool_cleanup(crsynctool_handle_t *handle);

CRSYNCcode crsynctool_main();

#if defined __cplusplus
    }
#endif

#endif // CRSYNC_TOOL_H
