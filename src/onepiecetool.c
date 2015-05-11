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

#include "onepiecetool.h"
#include "log.h"

#define Default_BLOCK_SIZE 8*1024 /*default block size to 8K*/

typedef struct onepiecetool_handle_t {
    magnet_t    *magnet;    /* magnet info */
    char        *app;    /* app */
    char        *resdir;    /* islands directory */
    char        *outputdir; /* output directory */
    uint32_t    block_sz;   /* block size */
} onepiecetool_handle_t;

static CRSYNCcode onepiecetool_movefile(const char *inputfile, const char *outputdir, const char *hash) {
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

onepiecetool_handle_t* onepiecetool_init() {
    onepiecetool_handle_t *handle = calloc(1, sizeof(onepiecetool_handle_t));
    onepiece_magnet_new(handle->magnet);
    handle->block_sz = Default_BLOCK_SIZE;
    return handle;
}

CRSYNCcode onepiecetool_setopt(onepiecetool_handle_t *handle, ONEPIECETOOLoption opt, ...) {
    if(!handle) {
        return CRSYNCE_INVALID_OPT;
    }

    CRSYNCcode code = CRSYNCE_OK;
    va_list arg;
    va_start(arg, opt);

    switch (opt) {
    case ONEPIECETOOLOPT_CURRID:
        if(handle->magnet->curr_id) {
            free(handle->magnet->curr_id);
        }
        handle->magnet->curr_id = strdup( va_arg(arg, const char*) );
        break;
    case ONEPIECETOOLOPT_NEXTID:
        if(handle->magnet->next_id) {
            free(handle->magnet->next_id);
        }
        handle->magnet->next_id = strdup( va_arg(arg, const char*) );
        break;
    case ONEPIECETOOLOPT_APP:
        if(handle->app) {
            free(handle->app);
        }
        handle->app = strdup( va_arg(arg, const char*) );
        break;
    case ONEPIECETOOLOPT_RESNAME:
    {
        const char *p = va_arg(arg, const char*);
        utarray_push_back(handle->magnet->resname, &p);
    }
        break;
    case ONEPIECETOOLOPT_RESDIR:
        if(handle->resdir) {
            free(handle->resdir);
        }
        handle->resdir = strdup( va_arg(arg, const char*) );
        break;
    case ONEPIECETOOLOPT_OUTPUT:
        if(handle->outputdir) {
            free(handle->outputdir);
        }
        handle->outputdir = strdup( va_arg(arg, const char*) );
        break;
    case ONEPIECETOOLOPT_BLOCKSIZE:
        handle->block_sz = va_arg(arg, uint32_t);
        break;
    default:
        code = CRSYNCE_INVALID_OPT;
        break;
    }
    va_end(arg);
    return code;
}

static CRSYNCcode onepiecetool_checkopt(onepiecetool_handle_t *handle) {
    if( handle &&
        handle->app &&
        handle->resdir &&
        handle->outputdir &&
        handle->block_sz > 0 &&
        handle->magnet->curr_id &&
        handle->magnet->next_id ) {
        return CRSYNCE_OK;
    }
    return CRSYNCE_INVALID_OPT;
}

CRSYNCcode onepiecetool_perform(onepiecetool_handle_t *handle) {
    CRSYNCcode code = CRSYNCE_OK;

    UT_string *input = NULL;
    utstring_new(input);

    UT_string *output = NULL;
    utstring_new(output);

    UT_string *hash = NULL;
    utstring_new(hash);

    char **p = NULL;

    do {
        code = onepiecetool_checkopt(handle);
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
        LOGI("perform app %s\n", handle->app);
        code = crsync_rsum_generate(handle->app, handle->block_sz, hash);
        if(CRSYNCE_OK != code) break;
        handle->magnet->apphash = strdup(utstring_body(hash));
        code = onepiecetool_movefile(handle->app, handle->outputdir, utstring_body(hash));
        if(CRSYNCE_OK != code) break;

        //generate base resource rsum file
        p = NULL;
        while ( (p=(char**)utarray_next(handle->magnet->resname,p))) {
            LOGI("perform res %s\n", *p);
            utstring_clear(input);
            utstring_printf(input, "%s%s", handle->resdir, *p);
            code = crsync_rsum_generate(utstring_body(input), handle->block_sz, hash);
            if(CRSYNCE_OK != code) break;
            utarray_push_back(handle->magnet->reshash, &utstring_body(hash));
            code = onepiecetool_movefile(utstring_body(input), handle->outputdir, utstring_body(hash));
            if(CRSYNCE_OK != code) break;
        }
        if(CRSYNCE_OK != code) break;

        //generate magnet info file
        utstring_clear(output);
        utstring_printf(output, "%s%s%s", handle->outputdir, handle->magnet->curr_id, MAGNET_SUFFIX);
        code = onepiece_magnet_generate(utstring_body(output), handle->magnet);
    } while (0);

    utstring_free(input);
    utstring_free(output);
    utstring_free(hash);

    return code;
}

void onepiecetool_cleanup(onepiecetool_handle_t *handle) {
    if(handle) {
        onepiece_magnet_free(handle->magnet);
        free(handle->app);
        free(handle->resdir);
        free(handle->outputdir);
        free(handle);
    }
}
