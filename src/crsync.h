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

#include "crsyncver.h"         /* crsync version defines   */

#include "uthash.h"
#include "utlist.h"
#include "utstring.h"
#include "tpl.h"
#include "curl/curl.h"

#define STRONG_SUM_SIZE 32  /*strong checksum size*/

#define RSUM_TPLMAP_FORMAT "uuc#BA(uc#)"
#define RSUM_SUFFIX ".rsum"

#define MSUM_TPLMAP_FORMAT "uuc#BA(uc#ui)"
#define MSUM_SUFFIX ".msum"

typedef struct rsum_meta_t{
    uint32_t    file_sz;                    /*file length*/
    uint32_t    block_sz;                   /*block length*/
    uint8_t     file_sum[STRONG_SUM_SIZE];  /*file strong sum*/
    tpl_bin     rest_tb;                    /*rest data*/
    uint32_t    same_count;                 /*same block count - memory usage*/
} rsum_meta_t;

typedef struct rsum_t {
    uint32_t        weak;                   /*first key*/
    uint32_t        seq;                    /*second key*/
    uint8_t         strong[STRONG_SUM_SIZE];/*value*/
    int32_t         offset;                 /*value*/
    struct rsum_t   *sub;                   /*sub HashTable*/
    UT_hash_handle  hh;
} rsum_t;

typedef enum {
    CRSYNCOPT_OUTPUTDIR = 1,/* local output dir */
    CRSYNCOPT_FILE,         /* local file name */
    CRSYNCOPT_HASH,         /* remote file's hash */
    CRSYNCOPT_BASEURL,      /* remote file's base url */
    CRSYNCOPT_XFER,         /* progress callback hook */
} CRSYNCoption;

typedef enum {
    CRSYNCE_OK = 0,
    CRSYNCE_FAILED_INIT,
    CRSYNCE_INVALID_OPT,
    CRSYNCE_FILE_ERROR,
    CRSYNCE_CURL_ERROR,
    CRSYNCE_CANCEL,

    CRSYNCE_BUG = 100,
} CRSYNCcode;

/*  transfer process callback
 *  params:
    hash: current file hash name
    percent: 0~100
    speed: %0.3f bytes/second
    return: 0 - go on; 1 - cancel
*/
typedef int (xfer_callback)(const char* hash, int percent, double speed);

typedef struct crsync_handle_t {
    rsum_meta_t     *meta;              /* file meta info */
    rsum_t          *sums;              /* rsum hash table */
    char            *outputdir;         /* local root dir */
    char            *fullFilename;      /* local file full name */
    char            *hash;              /* remote file hash */
    char            *baseurl;           /* remote base url */
    CURL            *curl_handle;       /* curl handle */
    uint8_t         *curl_buffer;       /* curl write callback data */
    uint32_t        curl_buffer_offset; /* curl write callback data offset */
    xfer_callback   *xfer;              /* progress callback hook */
} crsync_handle_t;

int xfer_default(const char *hash, int percent, double speed);

void rsum_weak_block(const uint8_t *data, uint32_t start, uint32_t block_sz, uint32_t *weak);
void rsum_weak_rolling(const uint8_t *data, uint32_t start, uint32_t block_sz, uint32_t *weak);
void rsum_strong_block(const uint8_t *p, uint32_t start, uint32_t block_sz, uint8_t *strong);
int rsum_strong_file(const char *file, uint8_t *strong);

CRSYNCcode crsync_rsum_generate(const char *filename, uint32_t blocksize, UT_string *hash);
UT_string* get_full_string(const char *base, const char *value, const char *suffix);
void crsync_curl_setopt(CURL *curlhandle);
CRSYNCcode crsync_tplfile_check(const char *filename, const char *sumfmt);

CRSYNCcode crsync_global_init();

/* return: NULL for fail */
crsync_handle_t* crsync_easy_init();

/* return: only CRSYNCE_OK or CRSYNC_INVALID_OPT */
CRSYNCcode crsync_easy_setopt(crsync_handle_t *handle, CRSYNCoption opt, ...);

CRSYNCcode crsync_easy_perform_match(crsync_handle_t *handle);
CRSYNCcode crsync_easy_perform_patch(crsync_handle_t *handle);

void crsync_easy_cleanup(crsync_handle_t *handle);

void crsync_global_cleanup();

#if defined __cplusplus
    }
#endif

#endif // CRSYNC_H
