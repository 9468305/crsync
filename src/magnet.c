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
#include <stdlib.h>
#include <string.h>

#include "magnet.h"
#include "utlist.h"
#include "tpl.h"
#include "util.h"
#include "log.h"

const char *MAGNET_EXT = ".fdi";
static const char *MAGNET_TPL_FORMAT = "ssA(suc#)";

sum_t* sum_malloc() {
    return calloc(1, sizeof(sum_t));
}

void sum_free(sum_t *s) {
    sum_t *elt, *tmp;
    LL_FOREACH_SAFE(s,elt,tmp) {
        LL_DELETE(s,elt);
        free(elt->name);
        free(elt);
    }
}

magnet_t* magnet_malloc() {
    return calloc(1, sizeof(magnet_t));
}

void magnet_free(magnet_t *m) {
    if(m) {
        free(m->currVersion);
        free(m->nextVersion);
        sum_free(m->file);
    }
}

void magnet_toString(magnet_t *m, UT_string **str) {
    if(!m || !str || !*str)
        return;
    UT_string *s = *str;
    utstring_clear(s);
    utstring_printf(s, "currentVersion = %s\n", m->currVersion);
    utstring_printf(s, "nextVersion = %s\n", m->nextVersion);
    utstring_printf(s, "files = ");

    sum_t *elt;
    LL_FOREACH(m->file, elt) {
        char *hash = Util_hex_string(elt->digest, CRS_STRONG_DIGEST_SIZE);
        utstring_printf(s, "%s;%s;%u;", elt->name, hash, elt->size);
        free(hash);
    }
}

magnet_t* magnet_fromString(UT_string **str) {
    if(!str || !*str) {
        return NULL;
    }
    magnet_t *m = magnet_malloc();
    UT_string *s = *str;
    int findPos = -1;
    int startPos = 0;
    int order = 0;

    sum_t *sum = NULL;
    char findStr[128];//should enough mostly

    while(-1 != (findPos = utstring_find(s, startPos, ";", 1))) {
        if(!sum)
            sum = sum_malloc();
        strncpy(findStr, utstring_body(s) + startPos, findPos - startPos);
        findStr[findPos - startPos] = '\0';

        switch(order) {
        case 0: {
            sum->name = strdup(findStr);
            break;
        }
        case 1: {
            unsigned char *digest = Util_string_hex(findStr);
            memcpy(sum->digest, digest, CRS_STRONG_DIGEST_SIZE);
            free(digest);
            break;
        }
        case 2: {
            sum->size = (unsigned int)atoi(findStr);
            LL_APPEND(m->file, sum);
            sum = NULL;
            break;
        }
        }
        order = (order >= 2) ? 0 : order+1;
        startPos = findPos + 1;
    }

    return m;
}

CRScode magnet_load(magnet_t *m, const char *file) {
    LOGI("begin\n");
    CRScode code = CRS_OK;
    char *name = NULL;
    unsigned int size = 0;
    unsigned char digest[CRS_STRONG_DIGEST_SIZE];
    tpl_node *tn = tpl_map(MAGNET_TPL_FORMAT,
                 &m->currVersion,
                 &m->nextVersion,
                 &name,
                 &size,
                 &digest,
                 CRS_STRONG_DIGEST_SIZE);
    if (0 == tpl_load(tn, TPL_FILE, file) ) {
        tpl_unpack(tn, 0);
        while (tpl_unpack(tn, 1) > 0) {
            sum_t *s = sum_malloc();
            s->name = strdup(name);
            s->size = size;
            memcpy(s->digest, digest, CRS_STRONG_DIGEST_SIZE);
            LL_APPEND(m->file, s);
        }
    } else {
        code = CRS_FILE_ERROR;
    }
    tpl_free(tn);
    LOGI("end %d\n", code);
    return code;
}

CRScode magnet_save(magnet_t *m, const char *file) {
    LOGI("begin\n");
    char *name = NULL;
    unsigned int size = 0;
    unsigned char digest[CRS_STRONG_DIGEST_SIZE];
    tpl_node *tn = tpl_map(MAGNET_TPL_FORMAT,
                 &m->currVersion,
                 &m->nextVersion,
                 &name,
                 &size,
                 &digest,
                 CRS_STRONG_DIGEST_SIZE);
    tpl_pack(tn, 0);

    sum_t *elt=NULL;
    LL_FOREACH(m->file,elt) {
        name = elt->name;
        size = elt->size;
        memcpy(digest,elt->digest, CRS_STRONG_DIGEST_SIZE);
        tpl_pack(tn, 1);
    }
    int result = tpl_dump(tn, TPL_FILE, file);
    tpl_free(tn);
    CRScode code = (-1 == result) ? CRS_FILE_ERROR : CRS_OK;
    LOGI("end %d\n", code);
    return code;
}

int Magnet_checkfile(const char *filename) {
    return Util_tplcmp(filename, MAGNET_TPL_FORMAT);
}
