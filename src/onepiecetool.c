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

static const uint32_t Default_BLOCK_SIZE = 8*1024; //default block size to 8K

onepiecetool_option_t* onepiecetool_option_malloc() {
    onepiecetool_option_t* op = calloc(1, sizeof(onepiecetool_option_t));
    op->block_size = Default_BLOCK_SIZE;
    return op;
}

void resname_free(resname_t *res) {
    resname_t *elt=NULL, *tmp=NULL;
    LL_FOREACH_SAFE(res, elt, tmp) {
        LL_DELETE(res, elt);
        free(elt->name);
        free(elt);
    }
}

void onepiecetool_option_free(onepiecetool_option_t *option) {
    if(option) {
        free(option->curr_id);
        free(option->next_id);
        free(option->app_name);
        free(option->res_dir);
        resname_free(option->res_list);
        free(option->output_dir);
        free(option);
    }
}

/* 1. copy input file to output dir with hashname
   2. move input file's rsum file to output dir with hashname.rsum
*/
static CRSYNCcode util_movefile(const char *inputfile, const char *outputdir, const char *hash) {
    CRSYNCcode code = CRSYNCE_FILE_ERROR;
    UT_string *input = NULL;
    utstring_new(input);
    utstring_printf(input, "%s%s", inputfile, RSUM_SUFFIX);

    UT_string *output = NULL;
    utstring_new(output);
    utstring_printf(output, "%s%s", outputdir, hash);

    do {
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

        utstring_printf(output, "%s", RSUM_SUFFIX);
        int i = rename(utstring_body(input), utstring_body(output));
        code = (0 == i) ? CRSYNCE_OK : CRSYNCE_FILE_ERROR;
    } while (0);

    utstring_free(input);
    utstring_free(output);
    return code;
}

CRSYNCcode onepiecetool_perform(onepiecetool_option_t *option) {
    LOGI("onepiecetool_perform\n");
    CRSYNCcode code = CRSYNCE_OK;

    UT_string *input = NULL;
    utstring_new(input);

    UT_string *output = NULL;
    utstring_new(output);

    UT_string *hash = NULL;
    utstring_new(hash);

    LOGI("clean up %s\n", option->output_dir);
    DIR *dirp = opendir(option->output_dir);
    if(dirp) {
        struct dirent *direntp = NULL;
        while ((direntp = readdir(dirp)) != NULL) {
            if(0 == strcmp(direntp->d_name, ".") ||
               0 == strcmp(direntp->d_name, "..")) {
                continue;
            }
            LOGD("remove %s\n", direntp->d_name);
            utstring_clear(output);
            utstring_printf(output, "%s%s", option->output_dir, direntp->d_name);
            remove(utstring_body(output));
        }
        closedir(dirp);
    } else {
#ifndef _WIN32
        mkdir(option->output_dir, S_IRUSR|S_IWUSR|S_IWGRP|S_IRGRP|S_IROTH);
#else
        mkdir(option->output_dir);
#endif
    }

    oldmagnet_t *magnet = onepiece_magnet_malloc();
    magnet->curr_id = strdup(option->curr_id);
    magnet->next_id = strdup(option->next_id);

    do {
        LOGI("generate app rsum file\n");
        LOGI("perform %s\n", option->app_name);
        code = crsync_rsum_generate(option->app_name, option->block_size, hash);
        if(CRSYNCE_OK != code) break;
        res_t *app = calloc(1, sizeof(res_t));
        app->hash = strdup(utstring_body(hash));
        LL_APPEND(magnet->app, app);
        code = util_movefile(option->app_name, option->output_dir, utstring_body(hash));
        if(CRSYNCE_OK != code) break;

        LOGI("generate resource rsum file\n");
        resname_t *elt=NULL;
        LL_FOREACH(option->res_list, elt) {
            LOGI("perform %s\n", elt->name);
            utstring_clear(input);
            utstring_printf(input, "%s%s", option->res_dir, elt->name);
            code = crsync_rsum_generate(utstring_body(input), option->block_size, hash);
            if(CRSYNCE_OK != code) break;
            struct stat file_info;
            stat(utstring_body(input), &file_info);
            code = util_movefile(utstring_body(input), option->output_dir, utstring_body(hash));
            if(CRSYNCE_OK != code) break;

            res_t *res = calloc(1, sizeof(res_t));
            res->name = strdup(elt->name);
            res->hash = strdup(utstring_body(hash));
            res->size = file_info.st_size;
            LL_APPEND(magnet->res_list, res);
        }
        if(CRSYNCE_OK != code) break;

        LOGI("generate magnet file\n");
        utstring_clear(output);
        utstring_printf(output, "%s%s%s", option->output_dir, option->curr_id, MAGNET_SUFFIX);
        code = onepiece_magnet_save(utstring_body(output), magnet);
    } while (0);

    onepiece_magnet_free(magnet);
    utstring_free(input);
    utstring_free(output);
    utstring_free(hash);
    LOGI("onepiecetool_perform code = %d\n", code);
    return code;
}
