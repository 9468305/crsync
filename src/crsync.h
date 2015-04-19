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
#ifndef CRSYNC_H
#define CRSYNC_H

#if defined __cplusplus
extern "C" {
#endif

typedef struct crsync_handle_t crsync_handle_t;
typedef void (crsync_xfer_fcn)(int percent);

typedef enum {
    CRSYNCACTION_INIT = 1,
    CRSYNCACTION_MATCH,
    CRSYNCACTION_PATCH,
    /* ... */
    CRSYNCACTION_CLEANUP,
} CRSYNCaction;

typedef enum {
    CRSYNCOPT_ROOT = 1,     /* local root dir */
    CRSYNCOPT_FILE,         /* local file name */
    CRSYNCOPT_URL,          /* remote file's url */
    CRSYNCOPT_SUMURL,       /* remote file's rsums url */
    CRSYNCOPT_XFER,         /* progress callback hook */
} CRSYNCoption;

typedef enum {
    CRSYNCE_OK = 0,
    CRSYNCE_FAILED_INIT = -1,
    CRSYNCE_INVALID_OPT = -2,
    CRSYNCE_FILE_ERROR = -3,
    CRSYNCE_CURL_ERROR = -4,
    /* ... */
    CRSYNCE_BUG
} CRSYNCcode;

CRSYNCcode crsync_global_init();

/* return: NULL for fail */
crsync_handle_t* crsync_easy_init();

/* return: only CRSYNCE_OK or CRSYNC_INVALID_OPT */
CRSYNCcode crsync_easy_setopt(crsync_handle_t *handle, CRSYNCoption opt, ...);

/* return CRSYNCcode (<=0) or CURLcode (>=0) */
CRSYNCcode crsync_easy_perform(crsync_handle_t *handle);

void crsync_easy_cleanup(crsync_handle_t *handle);

void crsync_global_cleanup();

#if defined __cplusplus
    }
#endif

#endif // CRSYNC_H
