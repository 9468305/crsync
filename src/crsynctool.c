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
#include "crsynctool.h"
#include "crsync.c"

#define Default_BLOCK_SIZE 2048 /*default block size to 2K*/

typedef struct crsynctool_handle_t {
    rsum_meta_t     meta;      /* file meta info */
    char            *root;      /* local root dir */
    char            *file;      /* local file name */
    uint32_t        blocksize;  /* block size */
} crsynctool_handle_t;

CRSYNCcode crsynctool_rsums_generate(crsynctool_handle_t *handle) {
    CRSYNCcode code = CRSYNCE_OK;
    UT_string *rsumsFilename = NULL;
    utstring_new(rsumsFilename);
    utstring_printf(rsumsFilename, "%s%s%s", handle->root, handle->file, RSUMS_SUFFIX);

    tpl_mmap_rec    rec = {-1, NULL, 0};

    if (0 == tpl_mmap_file(handle->file, &rec)) {
        handle->meta.file_sz = rec.text_sz;
        rsum_strong_block(rec.text, 0, rec.text_sz, handle->meta.file_sum);
        uint32_t blocks = handle->meta.file_sz / handle->meta.block_sz;
        uint32_t i = handle->meta.file_sz - blocks * handle->meta.block_sz;
        if (i > 0) {
            tpl_bin_malloc(&handle->meta.rest_tb, i);
            memcpy(handle->meta.rest_tb.addr, rec.text + rec.text_sz - i, i);
        }
        uint32_t weak = 0;
        uint8_t strong[STRONG_SUM_SIZE] = {0};
        tpl_node *tn = tpl_map(RSUMS_TPLMAP_FORMAT,
                     &handle->meta.file_sz,
                     &handle->meta.block_sz,
                     handle->meta.file_sum,
                     STRONG_SUM_SIZE,
                     &handle->meta.rest_tb,
                     &weak,
                     &strong,
                     STRONG_SUM_SIZE);
        tpl_pack(tn, 0);

        for (i = 0; i < blocks; i++) {
            rsum_weak_block(rec.text, i*handle->meta.block_sz, handle->meta.block_sz, &weak);
            rsum_strong_block(rec.text, i*handle->meta.block_sz, handle->meta.block_sz, strong);
            tpl_pack(tn, 1);
        }

        int result = tpl_dump(tn, TPL_FILE, utstring_body(rsumsFilename));
        code = (-1 == result) ? CRSYNCE_FILE_ERROR : CRSYNCE_OK;
        tpl_free(tn);

        tpl_unmap_file(&rec);
    } else {
        code = CRSYNCE_FILE_ERROR;
    }

    utstring_free(rsumsFilename);
    return code;
}

crsynctool_handle_t* crsynctool_easy_init() {
    crsynctool_handle_t *handle = NULL;
    do {
        handle = calloc(1, sizeof(crsynctool_handle_t));
        if(!handle) {
            break;
        }
        handle->meta.block_sz = Default_BLOCK_SIZE;
        return handle;
    } while (0);

    if(handle) {
        free(handle);
    }
    return NULL;
}

CRSYNCcode crsynctool_easy_setopt(crsynctool_handle_t *handle, CRSYNCTOOLoption opt, ...) {
    if(!handle) {
        return CRSYNCE_INVALID_OPT;
    }

    CRSYNCcode code = CRSYNCE_OK;
    va_list arg;
    va_start(arg, opt);

    switch (opt) {
    case CRSYNCTOOLOPT_ROOT:
        if(handle->root) {
            free(handle->root);
        }
        handle->root = strdup( va_arg(arg, const char*) );
        break;
    case CRSYNCTOOLOPT_FILE:
        if(handle->file) {
            free(handle->file);
        }
        handle->file = strdup( va_arg(arg, const char*) );
        break;
    case CRSYNCTOOLOPT_BLOCKSIZE:
        handle->meta.block_sz = va_arg(arg, uint32_t);
        break;
    default:
        code = CRSYNCE_INVALID_OPT;
        break;
    }
    va_end(arg);
    return code;
}

static CRSYNCcode crsynctool_checkopt(crsynctool_handle_t *handle) {
    if( handle &&
        handle->root &&
        handle->file) {
        return CRSYNCE_OK;
    }
    return CRSYNCE_INVALID_OPT;
}

CRSYNCcode crsynctool_easy_perform(crsynctool_handle_t *handle) {
    CRSYNCcode code = CRSYNCE_OK;
    do {
        code = crsynctool_checkopt(handle);
        if(CRSYNCE_OK != code) {
            break;
        }
        code = crsynctool_rsums_generate(handle);

    } while (0);

    return code;
}

void crsynctool_easy_cleanup(crsynctool_handle_t *handle) {
    if(handle) {
        tpl_bin_free(&handle->meta.rest_tb);
        free(handle->file);
        free(handle->root);
        free(handle);
    }
}
