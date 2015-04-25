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
#include "onepiece.h"
#include "log.h"

CRSYNCcode onepiece_magnet_load(const char *magnetFilename, magnet_t *magnet) {
    LOGI("onepiece_magnet_load\n");
    CRSYNCcode code = CRSYNCE_OK;
    char *resname = NULL, *reshash = NULL;
    tpl_node *tn = tpl_map(MAGNET_TPLMAP_FORMAT,
                 &magnet->curr_id,
                 &magnet->next_id,
                 &magnet->appname,
                 &magnet->apphash,
                 &resname,
                 &reshash);
    if (!tpl_load(tn, TPL_FILE, magnetFilename) ) {
        tpl_unpack(tn, 0);
        while (tpl_unpack(tn, 1) > 0) {
            utarray_push_back(magnet->resname, &resname);
            free(resname);
            utarray_push_back(magnet->reshash, &reshash);
            free(reshash);
        }
    } else {
        code = CRSYNCE_FILE_ERROR;
    }
    tpl_free(tn);
    LOGI("onepiece_magnet_load code = %d\n", code);
    return code;
}

CRSYNCcode onepiece_magnet_generate(const char *magnetFilename, magnet_t *magnet) {
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
