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
#include <dirent.h>
#include <sys/stat.h>
#include "crsynctool.h"
#include "utstring.h"
#include "log.h"

#define Default_BLOCK_SIZE 2048 /*default block size to 2K*/

typedef struct crsynctool_handle_t {
    crsync_magnet_t *magnet;    /* magnet info */
    char            *appdir;    /* app directory */
    char            *resdir;    /* islands directory */
    char            *outputdir; /* output directory */
    uint32_t        block_sz;   /* block size */
} crsynctool_handle_t;

static CRSYNCcode crsync_movefile(const char *inputfile, const char *outputdir, const char *hash) {
    CRSYNCcode code = CRSYNCE_FILE_ERROR;
    UT_string *input = NULL;
    utstring_new(input);
    utstring_printf(input, "%s%s", inputfile, RSUM_SUFFIX);

    UT_string *output = NULL;
    utstring_new(output);

    do {
        //copy input file to output dir with hashname
        utstring_printf(output, "%s%s", outputdir, hash);
        tpl_mmap_rec recin = {-1, NULL, 0};
        tpl_mmap_rec recout = {-1, NULL, 0};
        if(0 == tpl_mmap_file(inputfile, &recin)) {
            recout.fd = tpl_mmap_output_file(utstring_body(output), recin.text_sz, &recout.text);
            if(-1 != recout.fd) {
                memcpy(recout.text, recin.text, recin.text_sz);
                tpl_unmap_file(&recout);
                code = CRSYNCE_OK;
            }
            tpl_unmap_file(&recin);
        }
        if(CRSYNCE_OK != code) break;

        //move input file's rsum file to output dir with hashname.rsum
        utstring_printf(output, "%s", RSUM_SUFFIX);
        int i = rename(utstring_body(input), utstring_body(output));
        code = (0 == i) ? CRSYNCE_OK : CRSYNCE_FILE_ERROR;
    } while (0);

    utstring_free(input);
    utstring_free(output);
    return code;
}

static CRSYNCcode crsynctool_rsum_generate(const char *filename, uint32_t blocksize, UT_string *hash) {
    CRSYNCcode code = CRSYNCE_OK;
    UT_string *rsumFilename = NULL;
    utstring_new(rsumFilename);
    utstring_printf(rsumFilename, "%s%s", filename, RSUM_SUFFIX);
    tpl_mmap_rec    rec = {-1, NULL, 0};

    if (0 == tpl_mmap_file(filename, &rec)) {
        rsum_meta_t meta;
        meta.block_sz = blocksize;
        meta.file_sz = rec.text_sz;
        rsum_strong_block(rec.text, 0, rec.text_sz, meta.file_sum);
        utstring_clear(hash);
        for(int j=0; j < STRONG_SUM_SIZE; j++) {
            utstring_printf(hash, "%02x", meta.file_sum[j] );
        }
        uint32_t blocks = meta.file_sz / meta.block_sz;
        uint32_t rest = meta.file_sz - blocks * meta.block_sz;
        if (rest > 0) {
            tpl_bin_malloc(&meta.rest_tb, rest);
            memcpy(meta.rest_tb.addr, rec.text + rec.text_sz - rest, rest);
        }
        uint32_t weak = 0;
        uint8_t strong[STRONG_SUM_SIZE] = {0};
        tpl_node *tn = tpl_map(RSUM_TPLMAP_FORMAT,
                     &meta.file_sz,
                     &meta.block_sz,
                     meta.file_sum,
                     STRONG_SUM_SIZE,
                     &meta.rest_tb,
                     &weak,
                     &strong,
                     STRONG_SUM_SIZE);
        tpl_pack(tn, 0);

        for (uint32_t i = 0; i < blocks; i++) {
            rsum_weak_block(rec.text, i*meta.block_sz, meta.block_sz, &weak);
            rsum_strong_block(rec.text, i*meta.block_sz, meta.block_sz, strong);
            tpl_pack(tn, 1);
        }

        int result = tpl_dump(tn, TPL_FILE, utstring_body(rsumFilename));
        code = (-1 == result) ? CRSYNCE_FILE_ERROR : CRSYNCE_OK;
        tpl_free(tn);

        if(rest > 0) {
            tpl_bin_free(&meta.rest_tb);
        }

        tpl_unmap_file(&rec);
    } else {
        code = CRSYNCE_FILE_ERROR;
    }
    utstring_free(rsumFilename);
    return code;
}

static CRSYNCcode crsynctool_magnet_generate(const char *magnetFilename, crsync_magnet_t *magnet) {
    CRSYNCcode code = CRSYNCE_OK;
    char *resname = NULL, *reshash = NULL;
    char **p = NULL, **q = NULL;

    tpl_node *tn = tpl_map(MAGNET_TPLMAP_FORMAT,
                 &magnet->curr_id,
                 &magnet->next_id,
                 &magnet->appname,
                 &magnet->apphash,
                 &resname,
                 &reshash);
    tpl_pack(tn, 0);

    p = NULL;
    q = NULL;
    while ( (p=(char**)utarray_next(magnet->resname,p)) && (q=(char**)utarray_next(magnet->reshash,q))) {
        resname = *p;
        reshash = *q;
        tpl_pack(tn, 1);
    }
    int result = tpl_dump(tn, TPL_FILE, magnetFilename);
    code = (-1 == result) ? CRSYNCE_FILE_ERROR : CRSYNCE_OK;
    tpl_free(tn);
    return code;
}

crsynctool_handle_t* crsynctool_init() {
    crsynctool_handle_t *handle = calloc(1, sizeof(crsynctool_handle_t));
    crsync_magnet_new(handle->magnet);
    handle->block_sz = Default_BLOCK_SIZE;
    return handle;
}

CRSYNCcode crsynctool_setopt(crsynctool_handle_t *handle, CRSYNCTOOLoption opt, ...) {
    if(!handle) {
        return CRSYNCE_INVALID_OPT;
    }

    CRSYNCcode code = CRSYNCE_OK;
    va_list arg;
    va_start(arg, opt);

    switch (opt) {
    case CRSYNCTOOLOPT_CURRID:
        if(handle->magnet->curr_id) {
            free(handle->magnet->curr_id);
        }
        handle->magnet->curr_id = strdup( va_arg(arg, const char*) );
        break;
    case CRSYNCTOOLOPT_NEXTID:
        if(handle->magnet->next_id) {
            free(handle->magnet->next_id);
        }
        handle->magnet->next_id = strdup( va_arg(arg, const char*) );
        break;
    case CRSYNCTOOLOPT_APPNAME:
        if(handle->magnet->appname) {
            free(handle->magnet->appname);
        }
        handle->magnet->appname =strdup( va_arg(arg, const char*) );
        break;
    case CRSYNCTOOLOPT_RESNAME:
    {
        const char *p = va_arg(arg, const char*);
        utarray_push_back(handle->magnet->resname, &p);
    }
        break;
    case CRSYNCTOOLOPT_APPDIR:
        if(handle->appdir) {
            free(handle->appdir);
        }
        handle->appdir = strdup( va_arg(arg, const char*) );
        break;
    case CRSYNCTOOLOPT_RESDIR:
        if(handle->resdir) {
            free(handle->resdir);
        }
        handle->resdir = strdup( va_arg(arg, const char*) );
        break;
    case CRSYNCTOOLOPT_OUTPUT:
        if(handle->outputdir) {
            free(handle->outputdir);
        }
        handle->outputdir = strdup( va_arg(arg, const char*) );
        break;
    case CRSYNCTOOLOPT_BLOCKSIZE:
        handle->block_sz = va_arg(arg, uint32_t);
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
        handle->appdir &&
        handle->resdir &&
        handle->outputdir &&
        handle->block_sz >= Default_BLOCK_SIZE &&
        handle->magnet->curr_id &&
        handle->magnet->next_id &&
        handle->magnet->appname ) {
        return CRSYNCE_OK;
    }
    return CRSYNCE_INVALID_OPT;
}

CRSYNCcode crsynctool_perform(crsynctool_handle_t *handle) {
    CRSYNCcode code = CRSYNCE_OK;

    UT_string *input = NULL;
    utstring_new(input);

    UT_string *output = NULL;
    utstring_new(output);

    UT_string *hash = NULL;
    utstring_new(hash);

    char **p = NULL;

    do {
        code = crsynctool_checkopt(handle);
        if(CRSYNCE_OK != code) break;

        //empty output dir
        DIR *dirp = NULL;
        dirp = opendir(handle->outputdir);
        if(dirp) {
            struct dirent *direntp = NULL;
            while ((direntp = readdir(dirp)) != NULL) {
                if(0 == strcmp(direntp->d_name, ".") ||
                   0 == strcmp(direntp->d_name, "..")) {
                    continue;
                }
                LOGD("remove %s\n", direntp->d_name);
                utstring_clear(output);
                utstring_printf(output, "%s%s", handle->outputdir, direntp->d_name);
                remove(utstring_body(output));
            }
            closedir(dirp);
        } else {
#ifndef _WIN32
            mkdir(handle->outputdir, S_IRUSR|S_IWUSR|S_IWGRP|S_IRGRP|S_IROTH);
#else
            mkdir(handle->outputdir);
#endif
        }

        //generate app rsum file
        LOGI("perform app %s\n", handle->magnet->appname);
        utstring_clear(input);
        utstring_printf(input, "%s%s", handle->appdir, handle->magnet->appname);
        code = crsynctool_rsum_generate(utstring_body(input), handle->block_sz, hash);
        if(CRSYNCE_OK != code) break;
        handle->magnet->apphash = strdup(utstring_body(hash));
        code = crsync_movefile(utstring_body(input), handle->outputdir, utstring_body(hash));
        if(CRSYNCE_OK != code) break;

        //generate base resource rsum file
        p = NULL;
        while ( (p=(char**)utarray_next(handle->magnet->resname,p))) {
            LOGI("perform res %s\n", *p);
            utstring_clear(input);
            utstring_printf(input, "%s%s", handle->resdir, *p);
            code = crsynctool_rsum_generate(utstring_body(input), handle->block_sz, hash);
            if(CRSYNCE_OK != code) break;
            utarray_push_back(handle->magnet->reshash, &utstring_body(hash));
            code = crsync_movefile(utstring_body(input), handle->outputdir, utstring_body(hash));
            if(CRSYNCE_OK != code) break;
        }
        if(CRSYNCE_OK != code) break;

        //generate magnet info file
        utstring_clear(output);
        utstring_printf(output, "%s%s%s", handle->outputdir, handle->magnet->curr_id, MAGNET_SUFFIX);
        code = crsynctool_magnet_generate(utstring_body(output), handle->magnet);
    } while (0);

    utstring_free(input);
    utstring_free(output);
    utstring_free(hash);

    return code;
}

void crsynctool_cleanup(crsynctool_handle_t *handle) {
    if(handle) {
        crsync_magnet_free(handle->magnet);
        free(handle->appdir);
        free(handle->resdir);
        free(handle->outputdir);
        free(handle);
    }
}

CRSYNCcode crsynctool_main() {
    CRSYNCcode code = CRSYNCE_OK;
    crsynctool_handle_t *handle = crsynctool_init();
    if(handle) {
        crsynctool_setopt(handle, CRSYNCTOOLOPT_CURRID, "14012");
        crsynctool_setopt(handle, CRSYNCTOOLOPT_NEXTID, "14013");
        crsynctool_setopt(handle, CRSYNCTOOLOPT_APPNAME, "swordgame.apk");

        crsynctool_setopt(handle, CRSYNCTOOLOPT_RESNAME, "base.obb");
        crsynctool_setopt(handle, CRSYNCTOOLOPT_RESNAME, "chapter0.obb");
        crsynctool_setopt(handle, CRSYNCTOOLOPT_RESNAME, "chapter20.obb");
        crsynctool_setopt(handle, CRSYNCTOOLOPT_RESNAME, "chapter21.obb");
        crsynctool_setopt(handle, CRSYNCTOOLOPT_RESNAME, "chapter1.obb");

        crsynctool_setopt(handle, CRSYNCTOOLOPT_RESNAME, "chapter2.obb");
        crsynctool_setopt(handle, CRSYNCTOOLOPT_RESNAME, "chapter3.obb");
        crsynctool_setopt(handle, CRSYNCTOOLOPT_RESNAME, "chapter4.obb");
        crsynctool_setopt(handle, CRSYNCTOOLOPT_RESNAME, "chapter5.obb");
        crsynctool_setopt(handle, CRSYNCTOOLOPT_RESNAME, "chapter6.obb");
        crsynctool_setopt(handle, CRSYNCTOOLOPT_RESNAME, "chapter7.obb");
        crsynctool_setopt(handle, CRSYNCTOOLOPT_RESNAME, "chapter8.obb");
        crsynctool_setopt(handle, CRSYNCTOOLOPT_RESNAME, "chapter9.obb");
        crsynctool_setopt(handle, CRSYNCTOOLOPT_RESNAME, "chapter10.obb");
        crsynctool_setopt(handle, CRSYNCTOOLOPT_RESNAME, "chapter11.obb");
        crsynctool_setopt(handle, CRSYNCTOOLOPT_RESNAME, "chapter12.obb");
        crsynctool_setopt(handle, CRSYNCTOOLOPT_RESNAME, "chapter13.obb");
        crsynctool_setopt(handle, CRSYNCTOOLOPT_RESNAME, "chapter14.obb");

        crsynctool_setopt(handle, CRSYNCTOOLOPT_APPDIR, "../14012/apk/");
        crsynctool_setopt(handle, CRSYNCTOOLOPT_RESDIR, "../14012/obb_etc/");
        crsynctool_setopt(handle, CRSYNCTOOLOPT_OUTPUT, "../14012/crsync/");
        crsynctool_setopt(handle, CRSYNCTOOLOPT_BLOCKSIZE, 2048);

        code = crsynctool_perform(handle);
        crsynctool_cleanup(handle);
    } else {
        code = CRSYNCE_FAILED_INIT;
    }
    return code;
}
