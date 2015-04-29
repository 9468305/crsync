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

int showUsage() {
    LOGI("Usage:\n");
    LOGI("crsync onepiecetool curr_id next_id appname appdir resdir outputdir blocksize\n");
    LOGI("Note:\n");
    LOGI("dir should end with '/'\n");
    LOGI("blocksize is better to be 2048, 4096, 8092, 16184\n");
    return -1;
}

int main_onepiece(int argc, char **argv) {
    return CRSYNCE_OK;
}

int main_onepiecetool(int argc, char **argv) {
    if(argc < 9) {
        return showUsage();
    }
    int c = 2;
    const char *curr_id = argv[c++];
    const char *next_id = argv[c++];
    const char *appname = argv[c++];
    const char *appdir = argv[c++];
    const char *resdir = argv[c++];
    const char *output = argv[c++];
    uint32_t blocksize = atoi(argv[c++]);

    CRSYNCcode code = CRSYNCE_OK;
    onepiecetool_handle_t *handle = onepiecetool_init();
    if(handle) {
        onepiecetool_setopt(handle, ONEPIECETOOLOPT_CURRID, curr_id);
        onepiecetool_setopt(handle, ONEPIECETOOLOPT_NEXTID, next_id);
        onepiecetool_setopt(handle, ONEPIECETOOLOPT_APPNAME, appname);

        onepiecetool_setopt(handle, ONEPIECETOOLOPT_RESNAME, "base.obb");
        onepiecetool_setopt(handle, ONEPIECETOOLOPT_RESNAME, "chapter0.obb");
        onepiecetool_setopt(handle, ONEPIECETOOLOPT_RESNAME, "chapter20.obb");
        onepiecetool_setopt(handle, ONEPIECETOOLOPT_RESNAME, "chapter21.obb");
        onepiecetool_setopt(handle, ONEPIECETOOLOPT_RESNAME, "chapter1.obb");
        onepiecetool_setopt(handle, ONEPIECETOOLOPT_RESNAME, "chapter2.obb");
        onepiecetool_setopt(handle, ONEPIECETOOLOPT_RESNAME, "chapter3.obb");
        onepiecetool_setopt(handle, ONEPIECETOOLOPT_RESNAME, "chapter4.obb");
        onepiecetool_setopt(handle, ONEPIECETOOLOPT_RESNAME, "chapter5.obb");
        onepiecetool_setopt(handle, ONEPIECETOOLOPT_RESNAME, "chapter6.obb");
        onepiecetool_setopt(handle, ONEPIECETOOLOPT_RESNAME, "chapter7.obb");
        onepiecetool_setopt(handle, ONEPIECETOOLOPT_RESNAME, "chapter8.obb");
        onepiecetool_setopt(handle, ONEPIECETOOLOPT_RESNAME, "chapter9.obb");
        onepiecetool_setopt(handle, ONEPIECETOOLOPT_RESNAME, "chapter10.obb");
        onepiecetool_setopt(handle, ONEPIECETOOLOPT_RESNAME, "chapter11.obb");
        onepiecetool_setopt(handle, ONEPIECETOOLOPT_RESNAME, "chapter12.obb");
        onepiecetool_setopt(handle, ONEPIECETOOLOPT_RESNAME, "chapter13.obb");
        onepiecetool_setopt(handle, ONEPIECETOOLOPT_RESNAME, "chapter14.obb");

        onepiecetool_setopt(handle, ONEPIECETOOLOPT_APPDIR, appdir);
        onepiecetool_setopt(handle, ONEPIECETOOLOPT_RESDIR, resdir);
        onepiecetool_setopt(handle, ONEPIECETOOLOPT_OUTPUT, output);
        onepiecetool_setopt(handle, ONEPIECETOOLOPT_BLOCKSIZE, blocksize);

        code = onepiecetool_perform(handle);
        onepiecetool_cleanup(handle);
    } else {
        code = CRSYNCE_FAILED_INIT;
    }
    return code;
}

int main(int argc, char **argv) {
    for(int i=0; i<argc; i++) {
        LOGI("argv %d %s\n", i, argv[i]);
    }

    if(argc < 2) {
        return showUsage();
    }

    if(0 == strncmp(argv[1], cmd_onepiecetool, strlen(cmd_onepiecetool))) {
        return main_onepiecetool(argc, argv);
    } else if(0 == strncmp(argv[1], cmd_onepiece, strlen(cmd_onepiece))) {
        return main_onepiece(argc, argv);
    } else {
        return showUsage();
    }
}

#if defined __cplusplus
    }
#endif
