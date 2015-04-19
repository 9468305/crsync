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
#include "blake2.h"
#include "utstring.h"
#include "log.h"

#define NEW_SUFFIX ".new"

static const size_t MAX_CURL_WRITESIZE = 16*1024; /* curl write data buffer size */
static const int    MAX_CURL_RETRY = 5;

static void xfer_fcn(int percent) {
#if CRSYNC_DEBUG
    LOGD("default xfer percent: %d\n", percent);
#else
    (void)percent;
#endif
}

void rsum_weak_block(const uint8_t *data, uint32_t start, uint32_t block_sz, uint32_t *weak)
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

void rsum_weak_rolling(const uint8_t *data, uint32_t start, uint32_t block_sz, uint32_t *weak)
{
    uint32_t a = *weak & 0xffff;
    uint32_t b = *weak >> 16;
    const uint8_t *p = data + start;

    a += p[block_sz] - p[0];
    b += a - block_sz * p[0];

    *weak = (a & 0xffff) | (b << 16);
}

void rsum_strong_block(const uint8_t *p, uint32_t start, uint32_t block_sz, uint8_t *strong)
{
    blake2s_state ctx;
    blake2s_init(&ctx, STRONG_SUM_SIZE);
    blake2s_update(&ctx, p+start, block_sz);
    blake2s_final(&ctx, (uint8_t *)strong, STRONG_SUM_SIZE);
}

static UT_string* get_full_filename(crsync_handle_t *handle, const char *suffix) {
    UT_string *name = NULL;
    utstring_new(name);
    if(suffix) {
        utstring_printf(name, "%s%s%s", handle->root, handle->file, suffix);
    } else {
        utstring_printf(name, "%s%s", handle->root, handle->file);
    }
    return name;
}

static void crsync_curl_setopt(CURL *curlhandle) {
#if CRSYNC_DEBUG
    curl_easy_setopt(curlhandle, CURLOPT_VERBOSE, 1);
#endif
    curl_easy_setopt(curlhandle, CURLOPT_PROTOCOLS, CURLPROTO_HTTP); /* http protocol only */
    curl_easy_setopt(curlhandle, CURLOPT_FAILONERROR, 1); /* request failure on HTTP response >= 400 */
    curl_easy_setopt(curlhandle, CURLOPT_AUTOREFERER, 1); /* allow auto referer */
    curl_easy_setopt(curlhandle, CURLOPT_FOLLOWLOCATION, 1); /* allow follow location */
    curl_easy_setopt(curlhandle, CURLOPT_MAXREDIRS, 5); /* allow redir 5 times */
    curl_easy_setopt(curlhandle, CURLOPT_CONNECTTIMEOUT, 20); /* connection timeout 20s */
}

static CRSYNCcode crsync_sum_check(crsync_handle_t *handle, const char *sumfmt, const char *suffix) {
    CRSYNCcode  code = CRSYNCE_FILE_ERROR;
    UT_string   *filename = get_full_filename(handle, suffix);

    char *fmt = tpl_peek(TPL_FILE, utstring_body(filename));
    if(fmt) {
        if(0 == strncmp( fmt, sumfmt, strlen(sumfmt) )) {
            code = CRSYNCE_OK;
        }
        free(fmt);
    }

    utstring_free(filename);
    return code;
}

static void crsync_sum_free(crsync_handle_t *handle) {
    if(!handle) {
        return;
    }
    if(handle->meta) {
        tpl_bin_free(&handle->meta->rest_tb);
        free(handle->meta);
        handle->meta = NULL;
    }
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
        handle->sums = NULL;
    }
}

static int crsync_curl_progress(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    (void)ultotal;
    (void)ulnow;
    crsync_handle_t *handle = (crsync_handle_t *)clientp;
    int progress = (dltotal == 0) ? 0 : (dlnow * 100 / dltotal);
    handle->xfer(progress);
    return 0;
}

static CRSYNCcode crsync_rsum_curl(crsync_handle_t *handle) {
    CRSYNCcode  code = CRSYNCE_CURL_ERROR;
    UT_string   *rsumFilename = get_full_filename(handle, RSUM_SUFFIX);
    int         retry = MAX_CURL_RETRY;
    do {
        FILE *rsumFile = fopen(utstring_body(rsumFilename), "wb");
        if(rsumFile) {
            crsync_curl_setopt(handle->curl_handle);
            curl_easy_setopt(handle->curl_handle, CURLOPT_URL, handle->sum_url);
            curl_easy_setopt(handle->curl_handle, CURLOPT_HTTPGET, 1);
            curl_easy_setopt(handle->curl_handle, CURLOPT_WRITEFUNCTION, NULL);
            curl_easy_setopt(handle->curl_handle, CURLOPT_WRITEDATA, (void *)rsumFile);
            curl_easy_setopt(handle->curl_handle, CURLOPT_NOPROGRESS, 0);
            curl_easy_setopt(handle->curl_handle, CURLOPT_XFERINFOFUNCTION, (void *)crsync_curl_progress);
            curl_easy_setopt(handle->curl_handle, CURLOPT_XFERINFODATA, (void *)handle);

            CURLcode curlcode = curl_easy_perform(handle->curl_handle);
            fclose(rsumFile);
            if( CURLE_OK == curlcode) {
                if( CRSYNCE_OK == crsync_sum_check(handle, RSUM_TPLMAP_FORMAT, RSUM_SUFFIX) ) {
                    code = CRSYNCE_OK;
                    break;
                }
            }
        } else {
            code = CRSYNCE_FILE_ERROR;
            break;
        }
    } while(retry-- > 0);

    utstring_free(rsumFilename);
    return code;
}

static CRSYNCcode crsync_rsum_load(crsync_handle_t *handle) {
    CRSYNCcode code = CRSYNCE_FILE_ERROR;

    UT_string   *rsumFilename = get_full_filename(handle, RSUM_SUFFIX);
    uint32_t    weak = 0;
    uint8_t     strong[STRONG_SUM_SIZE] = {0};
    tpl_node    *tn = NULL;
    rsum_t      *sumItem = NULL, *sumTemp = NULL;

    crsync_sum_free(handle);
    handle->meta = calloc(1, sizeof(rsum_meta_t));

    tn = tpl_map(RSUM_TPLMAP_FORMAT,
                 &handle->meta->file_sz,
                 &handle->meta->block_sz,
                 handle->meta->file_sum,
                 STRONG_SUM_SIZE,
                 &handle->meta->rest_tb,
                 &weak,
                 &strong,
                 STRONG_SUM_SIZE);

    if (!tpl_load(tn, TPL_FILE, utstring_body(rsumFilename)) ) {
        tpl_unpack(tn, 0);
        uint32_t blocks = handle->meta->file_sz / handle->meta->block_sz;
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
        code = CRSYNCE_OK;
    }
    tpl_free(tn);

    utstring_free(rsumFilename);
    return code;
}

static CRSYNCcode crsync_msum_generate(crsync_handle_t *handle) {
    CRSYNCcode code = CRSYNCE_OK;

    UT_string *oldFilename = get_full_filename(handle, NULL);

    tpl_mmap_rec    rec = {-1, NULL, 0};
    uint32_t        weak = 0, i=0;
    uint8_t         strong[STRONG_SUM_SIZE] = {""};
    rsum_t          *sumItem = NULL, *sumIter = NULL, *sumTemp = NULL;

    if ( 0 == tpl_mmap_file(utstring_body(oldFilename), &rec) ) {
        rsum_weak_block(rec.text, 0, handle->meta->block_sz, &weak);

        while (1) {
            HASH_FIND_INT( handle->sums, &weak, sumItem );
            if(sumItem)
            {
                rsum_strong_block(rec.text, i, handle->meta->block_sz, strong);
                if (0 == memcmp(strong, sumItem->strong, STRONG_SUM_SIZE)) {
                    sumItem->offset = i;
                }
                HASH_ITER(hh, sumItem->sub, sumIter, sumTemp) {
                    if (0 == memcmp(strong, sumIter->strong, STRONG_SUM_SIZE)) {
                        sumIter->offset = i;
                    }
                }
            }

            if (i == rec.text_sz - handle->meta->block_sz) {
                break;
            }

            rsum_weak_rolling(rec.text, i++, handle->meta->block_sz, &weak);
        }
        tpl_unmap_file(&rec);
    }
    utstring_free(oldFilename);
    return code;
}

static CRSYNCcode crsync_msum_save(crsync_handle_t *handle) {
    CRSYNCcode code = CRSYNCE_OK;

    tpl_node    *tn = NULL;
    rsum_t      *sumItem=NULL, *sumTemp=NULL, *sumIter=NULL, *sumTemp2=NULL;
    uint32_t    weak = 0;
    uint8_t     strong[STRONG_SUM_SIZE] = {0};
    uint32_t    seq;
    int32_t     offset;

    UT_string *msumFilename = get_full_filename(handle, MSUM_SUFFIX);

    tn = tpl_map(MSUM_TPLMAP_FORMAT,
                 &handle->meta->file_sz,
                 &handle->meta->block_sz,
                 handle->meta->file_sum,
                 STRONG_SUM_SIZE,
                 &handle->meta->rest_tb,
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

    tpl_dump(tn, TPL_FILE, utstring_body(msumFilename));
    tpl_free(tn);

    utstring_free(msumFilename);
    return code;
}

static CRSYNCcode crsync_msum_load(crsync_handle_t *handle) {
    CRSYNCcode  code = CRSYNCE_OK;
    tpl_node    *tn;
    rsum_t      *sumItem=NULL, *sumTemp=NULL;
    uint32_t    weak=0;
    uint8_t     strong[STRONG_SUM_SIZE] = {0};
    uint32_t    seq;
    int32_t     offset;
    UT_string   *msumFilename = get_full_filename(handle, MSUM_SUFFIX);

    crsync_sum_free(handle);
    handle->meta = calloc(1, sizeof(rsum_meta_t));

    tn = tpl_map(MSUM_TPLMAP_FORMAT,
                 &handle->meta->file_sz,
                 &handle->meta->block_sz,
                 handle->meta->file_sum,
                 STRONG_SUM_SIZE,
                 &handle->meta->rest_tb,
                 &weak,
                 &strong,
                 STRONG_SUM_SIZE,
                 &seq,
                 &offset);

    if(!tpl_load(tn, TPL_FILE, utstring_body(msumFilename))) {
        tpl_unpack(tn, 0);

        uint32_t blocks = handle->meta->file_sz / handle->meta->block_sz;
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
        code = CRSYNCE_OK;
    }
    tpl_free(tn);

    utstring_free(msumFilename);
    return code;
}

static size_t crsync_msum_curl_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    crsync_handle_t *handle = (crsync_handle_t*)userp;
    if( handle->curl_buffer_offset + realsize <= MAX_CURL_WRITESIZE ) {
        memcpy(handle->curl_buffer + handle->curl_buffer_offset, contents, realsize);
        handle->curl_buffer_offset += realsize;
        return realsize;
    } else {
        return 0;
    }
}

static CURLcode crsync_msum_curl(crsync_handle_t *handle, rsum_t *sumItem, tpl_mmap_rec *recNew) {
    CURLcode    code = CURL_LAST;
    long        rangeFrom = sumItem->seq * handle->meta->block_sz;
    long        rangeTo = rangeFrom + handle->meta->block_sz - 1;
    uint8_t     strong[STRONG_SUM_SIZE];
    UT_string   *range = NULL;
    utstring_new(range);
    utstring_printf(range, "%ld-%ld", rangeFrom, rangeTo);

    int retry = MAX_CURL_RETRY;
    do {
        crsync_curl_setopt(handle->curl_handle);
        curl_easy_setopt(handle->curl_handle, CURLOPT_URL, handle->url);
        curl_easy_setopt(handle->curl_handle, CURLOPT_WRITEFUNCTION, (void*)crsync_msum_curl_callback);
        curl_easy_setopt(handle->curl_handle, CURLOPT_WRITEDATA, (void *)handle);
        curl_easy_setopt(handle->curl_handle, CURLOPT_RANGE, utstring_body(range));
        curl_easy_setopt(handle->curl_handle, CURLOPT_TIMEOUT, 10);
        curl_easy_setopt(handle->curl_handle, CURLOPT_NOPROGRESS, 1);
        curl_easy_setopt(handle->curl_handle, CURLOPT_XFERINFOFUNCTION, NULL);
        curl_easy_setopt(handle->curl_handle, CURLOPT_XFERINFODATA, NULL);
        handle->curl_buffer_offset = 0;

        code = curl_easy_perform(handle->curl_handle);
        if(CURLE_OK == code) {
            if( handle->curl_buffer_offset == handle->meta->block_sz ) {
                rsum_strong_block(handle->curl_buffer, 0, handle->curl_buffer_offset, strong);
                if(0 == memcmp(strong, sumItem->strong, STRONG_SUM_SIZE)) {
                    memcpy(recNew->text + rangeFrom, handle->curl_buffer, handle->curl_buffer_offset);
                    break;
                }
            }
        }
        code = CURL_LAST;
    } while(retry-- > 0);

    utstring_free(range);
    return code;
}

static CRSYNCcode crsync_msum_patch(crsync_handle_t *handle, rsum_t *sumItem, tpl_mmap_rec *recOld, tpl_mmap_rec *recNew, uint8_t *strong) {
    CRSYNCcode code = CRSYNCE_OK;
    rsum_strong_block(recNew->text, sumItem->seq * handle->meta->block_sz, handle->meta->block_sz, strong);
    if(0 != memcmp(strong, sumItem->strong, STRONG_SUM_SIZE)) {
        if( (sumItem->offset != -1) && (recOld->fd != -1) ) {
            memcpy(recNew->text + sumItem->seq * handle->meta->block_sz, recOld->text + sumItem->offset, handle->meta->block_sz);
        } else {
            code = crsync_msum_curl(handle, sumItem, recNew);
        }
    }
    return code;
}

static CRSYNCcode crsync_match(crsync_handle_t *handle) {
    CRSYNCcode  code = CRSYNCE_OK;
    do {
        //if msum file exist, load it.
        if(CRSYNCE_OK == crsync_sum_check(handle, MSUM_TPLMAP_FORMAT, MSUM_SUFFIX)) {
            code = crsync_msum_load(handle);
        } else { //generate msum from rsum
            //if rsum file not exist, curl it.
            if(CRSYNCE_OK != crsync_sum_check(handle, RSUM_TPLMAP_FORMAT, RSUM_SUFFIX)) {
                if( CRSYNCE_OK != (code = crsync_rsum_curl(handle)) ) {
                    break;
                }
            }
            //load rsum file
            if( CRSYNCE_OK != (code = crsync_rsum_load(handle)) ) {
                break;
            }
            //generate msum
            if( CRSYNCE_OK != (code = crsync_msum_generate(handle)) ) {
                break;
            }
            //save msum to file
            crsync_msum_save(handle);
        }
    } while(0);
    return code;
}

static CRSYNCcode crsync_patch(crsync_handle_t *handle) {
    CRSYNCcode code = CRSYNCE_OK;

    UT_string *oldFilename = get_full_filename(handle, NULL);
    UT_string *newFilename = get_full_filename(handle, NEW_SUFFIX);

    tpl_mmap_rec    recOld = {-1, NULL, 0};
    tpl_mmap_rec    recNew = {-1, NULL, 0};

    do {
        //old file exist or not is OK
        tpl_mmap_file(utstring_body(oldFilename), &recOld);
        //new file must be ready
        recNew.fd = tpl_mmap_output_file(utstring_body(newFilename), handle->meta->file_sz, &recNew.text);
        if( -1 == recNew.fd ) {
            code = CRSYNCE_FILE_ERROR;
            break;
        }
        recNew.text_sz = handle->meta->file_sz;
        uint32_t blocks = handle->meta->file_sz / handle->meta->block_sz;

        uint32_t count = 0;
        uint8_t strong[STRONG_SUM_SIZE];
        rsum_t *sumItem=NULL, *sumTemp=NULL, *sumIter=NULL, *sumTemp2=NULL;
        HASH_ITER(hh, handle->sums, sumItem, sumTemp) {
            code = crsync_msum_patch(handle, sumItem, &recOld, &recNew, strong);
            handle->xfer( count++ * 100 / blocks );
            if(CRSYNCE_OK != code) {
                break;
            }
            HASH_ITER(hh, sumItem->sub, sumIter, sumTemp2 ) {
                code = crsync_msum_patch(handle, sumIter, &recOld, &recNew, strong);
                handle->xfer( count++ * 100 / blocks );
                if(CRSYNCE_OK != code) {
                    break;
                }
            }
            if(CRSYNCE_OK != code) {
                break;
            }
        }
        if(CRSYNCE_OK == code) {
            if(handle->meta->rest_tb.sz > 0) {
                memcpy(recNew.text + handle->meta->file_sz - handle->meta->rest_tb.sz, handle->meta->rest_tb.addr, handle->meta->rest_tb.sz);
            }
            rsum_strong_block(recNew.text, 0, recNew.text_sz, strong);
            if(0 != memcmp(strong, handle->meta->file_sum, STRONG_SUM_SIZE)) {
                code = CRSYNCE_BUG;
            }
        }

    } while (0);

    tpl_unmap_file(&recNew);
    tpl_unmap_file(&recOld);

    if(CRSYNCE_OK == code) {
        remove(utstring_body(oldFilename));
        rename(utstring_body(newFilename), utstring_body(oldFilename));

        UT_string *rsumFilename = get_full_filename(handle, RSUM_SUFFIX);
        UT_string *msumFilename = get_full_filename(handle, MSUM_SUFFIX);
        remove(utstring_body(rsumFilename));
        remove(utstring_body(msumFilename));
        utstring_free(rsumFilename);
        utstring_free(msumFilename);
    }

    utstring_free(oldFilename);
    utstring_free(newFilename);

    return code;
}

CRSYNCcode crsync_global_init() {
    CRSYNCcode code = CRSYNCE_FAILED_INIT;
    if(CURLE_OK == curl_global_init(CURL_GLOBAL_DEFAULT)) {
        code = CRSYNCE_OK;
    }
    return code;
}

crsync_handle_t* crsync_easy_init() {
    crsync_handle_t *handle = NULL;
    do {
        handle = calloc(1, sizeof(crsync_handle_t));
        if(!handle) {
            break;
        }
        handle->curl_handle = curl_easy_init();
        if(!handle->curl_handle) {
            break;
        }
        handle->curl_buffer = malloc(MAX_CURL_WRITESIZE);
        if(!handle->curl_buffer) {
            break;
        }
        handle->action = CRSYNCACTION_INIT;
        handle->xfer = xfer_fcn;
        return handle;
    } while (0);

    if(handle) {
        if(handle->curl_handle) {
            curl_easy_cleanup(handle->curl_handle);
        }
        if(handle->curl_buffer) {
            free(handle->curl_buffer);
        }
        free(handle);
    }
    return NULL;
}

CRSYNCcode crsync_easy_setopt(crsync_handle_t *handle, CRSYNCoption opt, ...) {
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
        if(handle->sum_url) {
            free(handle->sum_url);
        }
        handle->sum_url = strdup( va_arg(arg, const char*) );
        break;
    case CRSYNCOPT_XFER:
        handle->xfer = va_arg(arg, crsync_xfer_fcn*);
        break;
    default:
        code = CRSYNCE_INVALID_OPT;
        break;
    }
    va_end(arg);
    return code;
}

static CRSYNCcode crsync_checkopt(crsync_handle_t *handle) {
    if( handle &&
        handle->root &&
        handle->file &&
        handle->url &&
        handle->sum_url) {
        return CRSYNCE_OK;
    }
    return CRSYNCE_INVALID_OPT;
}

CRSYNCcode crsync_easy_perform(crsync_handle_t *handle) {
    CRSYNCcode code = CRSYNCE_OK;
    do {
        code = crsync_checkopt(handle);
        if(CRSYNCE_OK != code) {
            break;
        }
        handle->action++;
        switch(handle->action) {
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

void crsync_easy_cleanup(crsync_handle_t *handle) {
    if(handle) {
        crsync_sum_free(handle);
        free(handle->file);
        free(handle->root);
        free(handle->url);
        free(handle->sum_url);
        if(handle->curl_handle) {
            curl_easy_cleanup(handle->curl_handle);
        }
        free(handle->curl_buffer);
        free(handle);
    }
}

void crsync_global_cleanup() {
    curl_global_cleanup();
}

CRSYNCcode crsync_main() {
    CRSYNCcode code = CRSYNCE_OK;

    return code;
}
