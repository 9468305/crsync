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

#include "curl/curl.h"

#if defined __cplusplus
extern "C" {
#endif

typedef struct rsum_meta_t rsum_meta_t;
typedef struct rsum_t rsum_t;
typedef struct crsync_handle crsync_handle;

void crsync_global_init();

crsync_handle* crsync_easy_init();

void crsync_easy_setopt(crsync_handle *handle);

void crsync_easy_perform(crsync_handle *handle);

void crsync_easy_cleanup(crsync_handle *handle);

void crsync_global_cleanup();

/* base on new file, generate rsums to file */
void crsync_rsums_generate(const char *filename, const char *rsumsFilename);

/* download rsums file from url */
CURLcode crsync_rsums_curl(const char *url, const char *rsumsFilename);

/* load rums meta data and default match rsums from rsums file  */
BOOL crsync_rsums_load(const char *rsumsFilename, rsum_meta_t *meta, rsum_t **msums);

/* base on old file, with rsums meta data and default match rsums, generate matched rsums */
void crsync_msums_generate(const char *oldFilename, rsum_meta_t *meta, rsum_t **msums);

/* load matched rsums from file */
BOOL crsync_msums_load(const char *msumsFilename, rsum_meta_t *meta, rsum_t **msums);

/* save matched rsums to file */
BOOL crsync_msums_save(const char *msumsFilename, rsum_meta_t *meta, rsum_t **msums);

/* base on old file, with matched rsums and new file URL, generate new file */
CURLcode crsync_msums_patch(const char *oldFilename, const char *newFilename, const char* newFileURL, rsum_meta_t *meta, rsum_t **msums);


/* server side:
 * generate rsums file for filename to outputDir;
 * copy filename to outpuDir;
 * rename filename to hashname at outputDir
*/
void crsync_server(const char *filename, const char *outputDir);

/* client side:
 * update filename to new filename;
 * use rsumsURL to run rsync rolling match
 * use newFileURL to download delta data
*/
void crsync_client(const char *oldFilename, const char *newFilename, const char *rsumsURL, const char *newFileURL);


#if defined __cplusplus
    }
#endif

#endif // CRSYNC_H
