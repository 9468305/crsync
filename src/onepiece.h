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
#ifndef CRSYNC_ONEPIECE_H
#define CRSYNC_ONEPIECE_H

#if defined __cplusplus
extern "C" {
#endif

#include "crsync.h"

#define MAGNET_SUFFIX ".magnet"
#define MAGNET_TPLMAP_FORMAT "sssA(ss)"

typedef struct magnet_t {
    char        *curr_id;   /* current magnet info id */
    char        *next_id;   /* next magnet info id */
    char        *apphash;   /* android apk hash */
    UT_array    *resname;   /* resources name */
    UT_array    *reshash;   /* resources hash */
} magnet_t;

#define onepiece_magnet_new(a) do {\
    a=calloc(1, sizeof(magnet_t));\
    utarray_new(a->resname,&ut_str_icd);\
    utarray_new(a->reshash,&ut_str_icd);\
} while(0)

#define onepiece_magnet_free(a) do {\
    free(a->curr_id);\
    free(a->next_id);\
    free(a->apphash);\
    utarray_free(a->resname);\
    utarray_free(a->reshash);\
    free(a);\
    a=NULL;\
} while(0)

CRSYNCcode onepiece_magnet_load(const char *magnetFilename, magnet_t *magnet);
CRSYNCcode onepiece_magnet_generate(const char *magnetFilename, magnet_t *magnet);

typedef struct onepiece_t onepiece_t;

typedef enum {
    ONEPIECEOPT_STARTID = 0,
    ONEPIECEOPT_BASEURL,
    ONEPIECEOPT_LOCALAPP,
    ONEPIECEOPT_LOCALRES,
    ONEPIECEOPT_XFER,
} ONEPIECEoption;

typedef enum {
    ONEPIECEINFO_QUERY = 0,
} ONEPIECEinfo;

CRSYNCcode onepiece_init();
CRSYNCcode onepiece_setopt(ONEPIECEoption opt, void *value);
UT_string* onepiece_getinfo(ONEPIECEinfo info);
CRSYNCcode onepiece_perform_query();
CRSYNCcode onepiece_perform_updateapp();
CRSYNCcode onepiece_perform_updateres();
void onepiece_cleanup();

#if defined __cplusplus
    }
#endif

#endif // CRSYNC_ONEPIECE_H
