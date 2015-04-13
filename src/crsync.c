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

#include "crsync.h"
#include "uthash.h"
#include "tpl.h"
#include "blake2.h"
#include "utstring.h"

#define STRONG_SUM_SIZE 32  /*strong checksum size*/
#define Default_BLOCK_SIZE 2048 /*default block size to 2K*/

static const char *RSUMS_TPLMAP_FORMAT = "uuc#BA(uc#)";
static const char *MSUMS_TPLMAP_FORMAT = "uuc#BA(uc#ui)";
static const char *RSUMS_SUFFIX = ".rsums";
static const char *MSUMS_SUFFIX = ".msums";
static const char *NEW_SUFFIX   = ".new";

static const size_t MAX_CURL_WRITESIZE = 16*1024; /* curl write data buffer size */

typedef struct rsum_meta_t {
    uint32_t    file_sz;                    /*file length*/
    uint32_t    block_sz;                   /*block length*/
    uint8_t     file_sum[STRONG_SUM_SIZE];  /*file strong sum*/
    tpl_bin     rest_tb;                    /*rest data*/
} rsum_meta_t;

typedef struct rsum_t {
    uint32_t        weak;                   /*first key*/
    uint32_t        seq;                    /*second key*/
    uint8_t         strong[STRONG_SUM_SIZE];/*value*/
    int32_t         offset;                 /*value*/
    struct rsum_t   *sub;                   /*sub HashTable*/
    UT_hash_handle  hh;
} rsum_t;

typedef struct crsync_handle {
    CRSYNCaction    action;
    rsum_meta_t     meta;               /* file meta info */
    rsum_t          *sums;              /* rsum hash table */
    char            *root;              /* local root dir */
    char            *file;              /* local file name */
    char            *url;               /* remote file's url */
    char            *sums_url;          /* remote file's rsums url */
    CURL            *curl_handle;       /* curl handle */
    tpl_bin         curl_buffer;        /* curl write callback data */
    uint32_t        curl_buffer_offset; /* curl write callback data offset */
} crsync_handle;

static void rsum_weak_block(const uint8_t *data, uint32_t start, uint32_t block_sz, uint32_t *weak)
{
    uint32_t i = 0, a = 0, b = 0;
    const uint8_t *p = data + start;
    for ( ; i < block_sz - 4; i += 4) {
        b += 4 * (a + p[i]) + 3 * p[i + 1] + 2 * p[i + 2] + p[i + 3];
        a += p[i + 0] + p[i + 1] + p[i + 2] + p[i + 3];
    }
    for ( ; i < block_sz; i++) {
        a += p[i];
        b += a;
    }
    *weak = (a & 0xffff) | (b << 16);
}

static void rsum_weak_rolling(const uint8_t *data, uint32_t start, uint32_t block_sz, uint32_t *weak)
{
    uint32_t a = *weak & 0xffff;
    uint32_t b = *weak >> 16;
    const uint8_t *p = data + start;

    a += p[block_sz] - p[0];
    b += a - block_sz * p[0];

    *weak = (a & 0xffff) | (b << 16);
}

static void rsum_strong_block(const uint8_t *p, uint32_t start, uint32_t block_sz, uint8_t *strong)
{
    blake2b_state ctx;
    blake2b_init(&ctx, STRONG_SUM_SIZE);
    blake2b_update(&ctx, p+start, block_sz);
    blake2b_final(&ctx, (uint8_t *)strong, STRONG_SUM_SIZE);
}

static void crsync_curl_setopt(CURL *curlhandle) {
    curl_easy_setopt(curlhandle, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(curlhandle, CURLOPT_PROTOCOLS, CURLPROTO_HTTP); /* http protocol only */
    curl_easy_setopt(curlhandle, CURLOPT_FAILONERROR, 1); /* request failure on HTTP response >= 400 */
    curl_easy_setopt(curlhandle, CURLOPT_AUTOREFERER, 1); /* allow auto referer */
    curl_easy_setopt(curlhandle, CURLOPT_FOLLOWLOCATION, 1); /* allow follow location */
    curl_easy_setopt(curlhandle, CURLOPT_MAXREDIRS, 5); /* allow redir 5 times */
    curl_easy_setopt(curlhandle, CURLOPT_CONNECTTIMEOUT, 20); /* connection timeout 20s */
}

static BOOL crsync_rsums_check(crsync_handle *handle, const char *rsumsfmt, const char *suffix) {
    BOOL        result = FALSE;
    UT_string   *filename = NULL;

    utstring_new(filename);
    utstring_printf(filename, "%s%s%s", handle->root, handle->file, suffix);

    char *fmt = tpl_peek(TPL_FILE, utstring_body(filename));
    if(fmt) {
        if(0 == strncmp( fmt, rsumsfmt, strlen(rsumsfmt) )) {
            result = TRUE;
        }
        free(fmt);
    }

    utstring_free(filename);
    return result;
}

static CRSYNCcode crsync_rsums_curl(crsync_handle *handle) {
    CRSYNCcode code = CRSYNCE_OK;
    UT_string *rsumsFilename = NULL;
    utstring_new(rsumsFilename);
    utstring_printf(rsumsFilename, "%s%s%s", handle->root, handle->file, RSUMS_SUFFIX);
    FILE *rsumsFile = fopen(utstring_body(rsumsFilename), "wb");
    if(rsumsFile) {
        crsync_curl_setopt(handle->curl_handle);
        curl_easy_setopt(handle->curl_handle, CURLOPT_URL, handle->sums_url);
        curl_easy_setopt(handle->curl_handle, CURLOPT_HTTPGET, 1);
        curl_easy_setopt(handle->curl_handle, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(handle->curl_handle, CURLOPT_WRITEDATA, (void *)rsumsFile);

        code = curl_easy_perform(handle->curl_handle);
        fclose(rsumsFile);
        if( CURLE_OK == code) {
            if( !crsync_rsums_check(handle, RSUMS_TPLMAP_FORMAT, RSUMS_SUFFIX) ) {
                code = CURL_LAST;
            }
        }
    } else {
        code = CURLE_WRITE_ERROR;
    }

    utstring_free(rsumsFilename);
    return code;
}

static CRSYNCcode crsync_rsums_load(crsync_handle *handle) {
    CRSYNCcode code = CRSYNCE_OK;

    UT_string *rsumsFilename = NULL;
    utstring_new(rsumsFilename);
    utstring_printf(rsumsFilename, "%s%s%s", handle->root, handle->file, RSUMS_SUFFIX);

    uint32_t    weak = 0;
    uint8_t     strong[STRONG_SUM_SIZE] = {""};
    tpl_node    *tn = NULL;
    rsum_t      *sumItem = NULL, *sumTemp = NULL;

    tn = tpl_map(RSUMS_TPLMAP_FORMAT,
                 &handle->meta.file_sz,
                 &handle->meta.block_sz,
                 handle->meta.file_sum,
                 STRONG_SUM_SIZE,
                 &handle->meta.rest_tb,
                 &weak,
                 &strong,
                 STRONG_SUM_SIZE);

    tpl_load(tn, TPL_FILE, utstring_body(rsumsFilename));
    tpl_unpack(tn, 0);

    uint32_t blocks = handle->meta.file_sz / handle->meta.block_sz;

    for (uint32_t i = 0; i < blocks; i++) {
        tpl_unpack(tn, 1);

        sumItem = calloc(1, sizeof(rsum_t));
        sumItem->weak = weak;
        sumItem->seq = i;
        memcpy(sumItem->strong, strong, STRONG_SUM_SIZE);
        sumItem->offset = -1;
        sumItem->sub = NULL;

        HASH_FIND_INT(handle->sums, &weak, sumTemp);
        if (sumTemp == NULL) {
            HASH_ADD_INT( handle->sums, weak, sumItem );
        } else {
            HASH_ADD_INT( sumTemp->sub, seq, sumItem );
        }
    }

    tpl_free(tn);

#if 0
    size_t size = HASH_OVERHEAD(hh, handle->sums);
    printf("hashtable memory size=%ld\n", size);
#endif
    utstring_free(rsumsFilename);
    return code;
}

static CRSYNCcode crsync_msums_generate(crsync_handle *handle) {
    CRSYNCcode code = CRSYNCE_OK;

    UT_string *oldFilename = NULL;
    utstring_new(oldFilename);
    utstring_printf(oldFilename, "%s%s", handle->root, handle->file);

    tpl_mmap_rec    rec = {-1, NULL, 0};
    uint32_t        weak = 0, i=0;
    uint8_t         strong[STRONG_SUM_SIZE] = {""};
    rsum_t          *sumItem = NULL, *sumIter = NULL, *sumTemp = NULL;

    if ( !tpl_mmap_file(utstring_body(oldFilename), &rec) ) {
        rsum_weak_block(rec.text, 0, handle->meta.block_sz, &weak);

        while (TRUE) {
            HASH_FIND_INT( handle->sums, &weak, sumItem );
            if(sumItem)
            {
                rsum_strong_block(rec.text, i, handle->meta.block_sz, strong);
                if (0 == memcmp(strong, sumItem->strong, STRONG_SUM_SIZE)) {
                    sumItem->offset = i;
                }
                HASH_ITER(hh, sumItem->sub, sumIter, sumTemp) {
                    if (0 == memcmp(strong, sumIter->strong, STRONG_SUM_SIZE)) {
                        sumIter->offset = i;
                    }
                }
            }

            if (i == rec.text_sz - handle->meta.block_sz) {
                break;
            }

            rsum_weak_rolling(rec.text, i++, handle->meta.block_sz, &weak);
        }
        tpl_unmap_file(&rec);
    }
    utstring_free(oldFilename);
    return code;
}

static CRSYNCcode crsync_msums_save(crsync_handle *handle) {
    CRSYNCcode code = CRSYNCE_OK;

    tpl_node    *tn = NULL;
    rsum_t      *sumItem=NULL, *sumTemp=NULL, *sumIter=NULL, *sumTemp2=NULL;
    uint32_t    weak = 0;
    uint8_t     strong[STRONG_SUM_SIZE] = {0};
    uint32_t    seq;
    int32_t     offset;

    UT_string *msumsFilename = NULL;
    utstring_new(msumsFilename);
    utstring_printf(msumsFilename, "%s%s%s", handle->root, handle->file, MSUMS_SUFFIX);

    tn = tpl_map(MSUMS_TPLMAP_FORMAT,
                 &handle->meta.file_sz,
                 &handle->meta.block_sz,
                 handle->meta.file_sum,
                 STRONG_SUM_SIZE,
                 &handle->meta.rest_tb,
                 &weak,
                 &strong,
                 STRONG_SUM_SIZE,
                 &seq,
                 &offset);
    tpl_pack(tn, 0);

    HASH_ITER(hh, handle->sums, sumItem, sumTemp) {
        weak = sumItem->weak;
        memcpy(strong, sumItem->strong, STRONG_SUM_SIZE);
        seq = sumItem->seq;
        offset = sumItem->offset;
        tpl_pack(tn, 1);
        HASH_ITER(hh, sumItem->sub, sumIter, sumTemp2 ) {
            weak = sumIter->weak;
            memcpy(strong, sumIter->strong, STRONG_SUM_SIZE);
            seq = sumIter->seq;
            offset = sumIter->offset;
            tpl_pack(tn, 1);
        }
    }

    tpl_dump(tn, TPL_FILE, utstring_body(msumsFilename));
    tpl_free(tn);

    utstring_free(msumsFilename);
    return code;
}

static CRSYNCcode crsync_msums_load(crsync_handle *handle) {
    CRSYNCcode code = CRSYNCE_OK;

    tpl_node    *tn;
    rsum_t      *sumItem=NULL, *sumTemp=NULL;
    uint32_t    weak=0;
    uint8_t     strong[STRONG_SUM_SIZE] = {0};
    uint32_t    seq;
    int32_t     offset;

    UT_string *msumsFilename = NULL;
    utstring_new(msumsFilename);
    utstring_printf(msumsFilename, "%s%s%s", handle->root, handle->file, MSUMS_SUFFIX);

    tn = tpl_map(MSUMS_TPLMAP_FORMAT,
                 &handle->meta.file_sz,
                 &handle->meta.block_sz,
                 handle->meta.file_sum,
                 STRONG_SUM_SIZE,
                 &handle->meta.rest_tb,
                 &weak,
                 &strong,
                 STRONG_SUM_SIZE,
                 &seq,
                 &offset);

    tpl_load(tn, TPL_FILE, msumsFilename);

    tpl_unpack(tn, 0);

    uint32_t blocks = handle->meta.file_sz / handle->meta.block_sz;

    for (uint32_t i = 0; i < blocks; i++) {
        tpl_unpack(tn, 1);

        sumItem = calloc(1, sizeof(rsum_t));
        sumItem->weak = weak;
        sumItem->seq = seq;
        memcpy(sumItem->strong, strong, STRONG_SUM_SIZE);
        sumItem->offset = offset;
        sumItem->sub = NULL;

        HASH_FIND_INT(handle->sums, &weak, sumTemp);
        if (sumTemp == NULL) {
            HASH_ADD_INT( handle->sums, weak, sumItem );
        } else {
            HASH_ADD_INT( sumTemp->sub, seq, sumItem );
        }
    }

    tpl_free(tn);

    utstring_free(msumsFilename);
    return code;
}

static size_t crsync_msums_curl_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    crsync_handle *handle = (crsync_handle*)userp;
    if( handle->curl_buffer_offset + realsize <= handle->curl_buffer.sz ) {
        memcpy(handle->curl_buffer.addr + handle->curl_buffer_offset, contents, realsize);
        handle->curl_buffer_offset += realsize;
        return realsize;
    } else {
        return 0;
    }
}

static CURLcode crsync_msums_curl(crsync_handle *handle, rsum_t *sumItem, tpl_mmap_rec *recNew) {
    CRSYNCcode code = CURL_LAST;
    uint32_t    rangeFrom = sumItem->seq * handle->meta.block_sz;
    uint32_t    rangeTo = rangeFrom + handle->meta.block_sz - 1;
    UT_string *range = NULL;
    utstring_new(range);
    utstring_printf(range, "%u-%u", rangeFrom, rangeTo);

    crsync_curl_setopt(handle->curl_handle);
    curl_easy_setopt(handle->curl_handle, CURLOPT_URL, handle->url);
    curl_easy_setopt(handle->curl_handle, CURLOPT_WRITEFUNCTION, (void*)crsync_msums_curl_callback);
    curl_easy_setopt(handle->curl_handle, CURLOPT_WRITEDATA, (void *)handle);
    curl_easy_setopt(handle->curl_handle, CURLOPT_RANGE, utstring_body(range));

    if(CURLE_OK == curl_easy_perform(handle->curl_handle)) {
        if( handle->curl_buffer_offset == handle->meta.block_sz ) {
            uint8_t strong[STRONG_SUM_SIZE];
            rsum_strong_block(handle->curl_buffer.addr, 0, handle->curl_buffer_offset, strong);
            if(0 == memcmp(strong, sumItem->strong, STRONG_SUM_SIZE)) {
                memcpy(recNew->text + rangeFrom, handle->curl_buffer.addr, handle->curl_buffer_offset);
                code = CRSYNCE_OK;
            }
        }
    }
    handle->curl_buffer_offset = 0;

    utstring_free(range);
    return code;
}

static CRSYNCcode crsync_hatch(crsync_handle *handle) {
    CRSYNCcode code = CRSYNCE_OK;
    if( !crsync_rsums_check(handle, RSUMS_TPLMAP_FORMAT, RSUMS_SUFFIX) ) {
        code = crsync_rsums_curl(handle);
    }
    return code;
}

static CRSYNCcode crsync_match(crsync_handle *handle) {
    CRSYNCcode code = CRSYNCE_OK;

    if( !crsync_rsums_check(handle, MSUMS_TPLMAP_FORMAT, MSUMS_SUFFIX) ) {
        crsync_rsums_load(handle);
        crsync_msums_generate(handle);
        crsync_msums_save(handle);
    } else {
        crsync_msums_load(handle);
    }

    return code;
}

static CRSYNCcode crsync_patch(crsync_handle *handle) {
    CRSYNCcode code = CRSYNCE_OK;

    UT_string *oldFilename = NULL;
    utstring_new(oldFilename);
    utstring_printf(oldFilename, "%s%s", handle->root, handle->file);

    UT_string *newFilename = NULL;
    utstring_new(newFilename);
    utstring_printf(newFilename, "%s%s%s", handle->root, handle->file, NEW_SUFFIX);

    tpl_mmap_rec    recOld = {-1, NULL, 0};
    tpl_mmap_rec    recNew = {-1, NULL, 0};
    rsum_t          *sumItem=NULL, *sumTemp=NULL, *sumIter=NULL, *sumTemp2=NULL;

    tpl_mmap_file(utstring_body(oldFilename), &recOld);

    recNew.text_sz = handle->meta.file_sz;
    tpl_mmap_file_output(utstring_body(newFilename), &recNew);

    HASH_ITER(hh, handle->sums, sumItem, sumTemp) {
        if(sumItem->offset == -1) {
            crsync_msums_curl(handle, sumItem, &recNew);
        } else {
            memcpy(recNew.text + sumItem->seq * handle->meta.block_sz, recOld.text + sumItem->offset, handle->meta.block_sz);
        }
        HASH_ITER(hh, sumItem->sub, sumIter, sumTemp2 ) {
            if(sumIter->offset == -1) {
                crsync_msums_curl(handle, sumIter, &recNew);
            } else {
                memcpy(recNew.text + sumIter->seq * handle->meta.block_sz, recOld.text + sumIter->offset, handle->meta.block_sz);
            }
        }
    }
    if(handle->meta.rest_tb.sz > 0) {
        memcpy(recNew.text + handle->meta.file_sz - handle->meta.rest_tb.sz, handle->meta.rest_tb.addr, handle->meta.rest_tb.sz);
    }

    tpl_unmap_file(&recNew);
    tpl_unmap_file(&recOld);

    remove(utstring_body(oldFilename));
    rename(utstring_body(newFilename), utstring_body(oldFilename));

    utstring_free(oldFilename);
    utstring_free(newFilename);

    return code;
}

CRSYNCcode crsync_global_init() {
    if(CURLE_OK == curl_global_init(CURL_GLOBAL_DEFAULT)) {
        return CRSYNCE_OK;
    } else {
        return CRSYNCE_FAILED_INIT;
    }
}

crsync_handle* crsync_easy_init() {
    crsync_handle *handle = NULL;
    do {
        handle = calloc(1, sizeof(crsync_handle));
        if(!handle) {
            break;
        }
        handle->curl_handle = curl_easy_init();
        if(!handle->curl_handle) {
            break;
        }
        tpl_bin_malloc(&handle->curl_buffer, MAX_CURL_WRITESIZE);
        handle->action = CRSYNCACTION_INIT;
        return handle;
    } while (0);

    if(handle) {
        if(handle->curl_handle) {
            curl_easy_cleanup(handle->curl_handle);
        }
        tpl_bin_free(&handle->curl_buffer);
        free(handle);
    }
    return NULL;
}

CRSYNCcode crsync_easy_setopt(crsync_handle *handle, CRSYNCoption opt, ...) {
    if(!handle) {
        return CRSYNCE_INVALID_OPT;
    }

    CRSYNCcode code = CRSYNCE_OK;
    va_list arg;
    va_start(arg, opt);

    switch (opt) {
    case CRSYNCOPT_ROOT:
        if(handle->root) {
            free(handle->root);
        }
        handle->root = strdup( va_arg(arg, const char*) );
        break;
    case CRSYNCOPT_FILE:
        if(handle->file) {
            free(handle->file);
        }
        handle->file = strdup( va_arg(arg, const char*) );
        break;
    case CRSYNCOPT_URL:
        if(handle->url) {
            free(handle->url);
        }
        handle->url = strdup( va_arg(arg, const char*) );
        break;
    case CRSYNCOPT_SUMURL:
        if(handle->sums_url) {
            free(handle->sums_url);
        }
        handle->sums_url = strdup( va_arg(arg, const char*) );
        break;
    default:
        code = CRSYNCE_INVALID_OPT;
    }
    va_end(arg);
    return code;
}

static BOOL crsync_checkopt(crsync_handle *handle) {
    if( handle &&
        handle->file &&
        handle->url &&
        handle->sums_url) {
        return TRUE;
    }
    return FALSE;
}
/*
    crsync_rsums_curl(rsumsURL, rsumsFilename);
    crsync_rsums_load(rsumsFilename, &meta, &msums);
    crsync_msums_generate(oldFilename, &meta, &msums);
    crsync_msums_save(msumsFilename, &meta, &msums);
    crsync_rsums_free(&meta, &msums);

    crsync_msums_load(msumsFilename, &meta, &msums);
    crsync_msums_patch(oldFilename, newFilename, newFileURL, &meta, &msums);

    crsync_rsums_free(&meta, &msums);

*/
CRSYNCcode crsync_easy_perform(crsync_handle *handle) {
    CRSYNCcode code = CRSYNCE_OK;
    do {
        if(!crsync_checkopt(handle)) {
            code = CRSYNCE_INVALID_OPT;
            break;
        }
        handle->action++;
        switch(handle->action) {
        case CRSYNCACTION_HATCH:
            code = crsync_hatch(handle);
            break;
        case CRSYNCACTION_MATCH:
            code = crsync_match(handle);
            break;
        case CRSYNCACTION_PATCH:
            code = crsync_patch(handle);
            break;
        default:
            break;
        }
    } while (0);

    return code;
}

void crsync_easy_cleanup(crsync_handle *handle) {
    if(handle) {
        tpl_bin_free(&handle->meta.rest_tb);
        if(handle->sums) {
            rsum_t *sumItem=NULL, *sumTemp=NULL, *sumIter=NULL, *sumTemp2=NULL;
            HASH_ITER(hh, handle->sums, sumItem, sumTemp) {
                HASH_ITER(hh, sumItem->sub, sumIter, sumTemp2 ) {
                    HASH_DEL(sumItem->sub, sumIter);
                    free(sumIter);
                }
                HASH_DEL(handle->sums, sumItem);
                free(sumItem);
            }
        }
        free(handle->file);
        free(handle->root);
        free(handle->url);
        free(handle->sums_url);
        if(handle->curl_handle) {
            curl_easy_cleanup(handle->curl_handle);
        }
        tpl_bin_free(&handle->curl_buffer);
        free(handle);
    }
}

void crsync_global_cleanup() {
    curl_global_cleanup();
}

static void crsync_rsums_free(rsum_meta_t *meta, rsum_t **rsums) {
    rsum_t *sumItem=NULL, *sumTemp=NULL, *sumIter=NULL, *sumTemp2=NULL;

    HASH_ITER(hh, *rsums, sumItem, sumTemp) {
        HASH_ITER(hh, sumItem->sub, sumIter, sumTemp2 ) {
            HASH_DEL(sumItem->sub, sumIter);
            free(sumIter);
        }
        HASH_DEL(*rsums, sumItem);
        free(sumItem);
    }

    tpl_bin_free(&meta->rest_tb);
}

void crsync_rsums_generate(const char *filename, const char *rsumsFilename) {
    tpl_mmap_rec    rec = {-1, NULL, 0};
    rsum_meta_t     meta = {0, 0, "", {NULL, 0}};
    uint32_t        weak = 0, blocks=0, i=0;
    uint8_t         strong[STRONG_SUM_SIZE] = {""};
    tpl_node        *tn = NULL;
    int             result=0;

    result = tpl_mmap_file(filename, &rec);
    if (!result) {
        meta.file_sz = rec.text_sz;
        meta.block_sz = Default_BLOCK_SIZE;
        rsum_strong_block(rec.text, 0, rec.text_sz, meta.file_sum);
        blocks = meta.file_sz / meta.block_sz;
        i = meta.file_sz - blocks * meta.block_sz;
        if (i > 0) {
            tpl_bin_malloc(&meta.rest_tb, i);
            memcpy(meta.rest_tb.addr, rec.text + rec.text_sz - i, i);
        }

        tn = tpl_map(RSUMS_TPLMAP_FORMAT,
                     &meta.file_sz,
                     &meta.block_sz,
                     meta.file_sum,
                     STRONG_SUM_SIZE,
                     &meta.rest_tb,
                     &weak,
                     &strong,
                     STRONG_SUM_SIZE);
        tpl_pack(tn, 0);

        for (i = 0; i < blocks; i++) {
            rsum_weak_block(rec.text, i*meta.block_sz, meta.block_sz, &weak);
            rsum_strong_block(rec.text, i*meta.block_sz, meta.block_sz, strong);
            tpl_pack(tn, 1);
        }

        tpl_dump(tn, TPL_FILE, rsumsFilename);
        tpl_free(tn);

        tpl_unmap_file(&rec);
    }

    rsum_t *rsums = NULL;
    crsync_rsums_free(&meta, &rsums);
}

void crsync_server(const char *filename, const char *outputDir) {

    //TODO: complete all logic
    crsync_rsums_generate(filename, "mthd.apk.rsums");
}

void crsync_client(const char *filename, const char *rsumsURL, const char *newFileURL) {

    crsync_global_init();
    crsync_handle* handle = crsync_easy_init();
    if(handle) {
        crsync_easy_setopt(handle, CRSYNCOPT_ROOT, "./");
        crsync_easy_setopt(handle, CRSYNCOPT_FILE, filename);
        crsync_easy_setopt(handle, CRSYNCOPT_URL, newFileURL);
        crsync_easy_setopt(handle, CRSYNCOPT_SUMURL, rsumsURL);

        crsync_easy_perform(handle);
        printf("1\n");
        crsync_easy_perform(handle);
        printf("2\n");
        crsync_easy_perform(handle);
        printf("3\n");

        crsync_easy_cleanup(handle);
    }
    crsync_global_cleanup();
}
