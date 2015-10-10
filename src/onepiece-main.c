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
#if defined __cplusplus
extern "C" {
#endif

#include "log.h"
#include "onepiecetool.h"

static const char *cmd_onepiecetool = "onepiecetool";
static const char *cmd_onepiece = "onepiece";

static void showUsage() {
    LOGI("crsync Usage:\n");
    LOGI("crsync [option] [parameters]\n");
    LOGI("Options:\n");
    LOGI("    onepiecetool : generate resource meta info (magnet, rsum, file_hash)\n");
    LOGI("    onepiece     : perform resource rsync update\n");
}

static void showUsage_onepiecetool() {
    LOGI("onepiecetool Usage:\n");
    LOGI("crsync onepiecetool [parameters]\n");
    LOGI("    curr_id\n"
         "    next_id\n"
         "    app_fullname\n"
         "    res_name_file\n"
         "    res_dir\n"
         "    output_dir\n"
         "    blocksize\n");
    LOGI("Note:\n"
         "    dir should end with '/'\n"
         "    res_name_file is a text file contains resname at every line\n"
         "    blocksize is better to be 8-8092, 16-16184, 32-32768\n");
}

static void showUsage_onepiece() {
    LOGI("onepiece Usage:\n");
    LOGI("crsync onepiece curr_id \n");
}

static void util_setopt_resfile(onepiecetool_option_t *option, const char *resfile) {
    FILE *f = NULL;
    const uint32_t size = 512;
    char line[size];
    f = fopen(resfile, "rt");
    if(f) {
        while(NULL != fgets(line, size, f)) {
            size_t len = strlen(line);
            if(len > 0) {
                if(line[len-1] == '\n') {
                    line[len-1] = '\0';
                }
                resname_t *res = calloc(1, sizeof(resname_t));
                res->name = strdup(line);
                LL_APPEND(option->res_list, res);
            }
        }
        fclose(f);
    }
}

int main_onepiece(int argc, char **argv) {
    if(argc != 6) {
        showUsage_onepiece();
        return -1;
    }/*
    int c = 0;
    c++;//crsync_exe
    c++;//operation is onepiece
    const char *curr_id = argv[c++];
    const char *baseUrl = argv[c++];
    const char *localApp = argv[c++];
    const char *localRes = argv[c++];
    CRSYNCcode code = onepiece_init();
    if(CRSYNCE_OK != code) {
        return code;
    }
    do {
        code = onepiece_setopt(ONEPIECEOPT_STARTID, (void*)curr_id);
        if(CRSYNCE_OK != code) break;
        code = onepiece_setopt(ONEPIECEOPT_BASEURL, (void*)baseUrl);
        if(CRSYNCE_OK != code) break;
        code = onepiece_setopt(ONEPIECEOPT_LOCALAPP, (void*)localApp);
        if(CRSYNCE_OK != code) break;
        code = onepiece_setopt(ONEPIECEOPT_LOCALRES, (void*)localRes);
        if(CRSYNCE_OK != code) break;

        code = onepiece_perform_query();
        if(CRSYNCE_OK != code) break;
        magnet_t *magnet = NULL;
        code = onepiece_getinfo(ONEPIECEINFO_MAGNET, (void**)&magnet);
        //TODO: parse query result
        //TODO: updateApp
        //TODO: updateRes
    } while(0);
    onepiece_cleanup();*/
    return -1;
}

int main_onepiecetool(int argc, char **argv) {
    if(argc != 9) {
        showUsage_onepiecetool();
        return -1;
    }
    onepiecetool_option_t *option = onepiecetool_option_malloc();
    int c = 0;
    c++;//crsync_exe
    c++;//operation is onepiecetool
    option->curr_id = strdup(argv[c++]);
    option->next_id = strdup(argv[c++]);
    option->app_name = strdup(argv[c++]);
    util_setopt_resfile(option, argv[c++]);
    option->res_dir = strdup(argv[c++]);
    option->output_dir = strdup(argv[c++]);
    option->block_size = atoi(argv[c++]) * 1024;

    int code = onepiecetool_perform(option);
    onepiecetool_option_free(option);
    return code;
}

int main(int argc, char **argv) {
    for(int i=0; i<argc; i++) {
        LOGI("argv %d %s\n", i, argv[i]);
    }

    if(argc < 2) {
        showUsage();
        return -1;
    }

    if(0 == strncmp(argv[1], cmd_onepiecetool, strlen(cmd_onepiecetool))) {
        return main_onepiecetool(argc, argv);
    } else if(0 == strncmp(argv[1], cmd_onepiece, strlen(cmd_onepiece))) {
        return main_onepiece(argc, argv);
    } else {
        showUsage();
        return -1;
    }
}

#if defined __cplusplus
    }
#endif
