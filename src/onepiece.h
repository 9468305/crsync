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

typedef struct res_t {
    char            *name;
    char            *hash;
    uint32_t        size;
    uint32_t        diff_size;
    struct res_t    *next;
} res_t;

void res_free(res_t *res);

typedef struct magnet_t {
    char    *curr_id;   /* current magnet info id */
    char    *next_id;   /* next magnet info id */
    res_t   *app;       /* android apk hash */
    res_t   *res_list;  /* resource list */
} magnet_t;

magnet_t* onepiece_magnet_malloc();
void onepiece_magnet_free(magnet_t *m);

CRSYNCcode onepiece_magnet_load(const char *filename, magnet_t *magnet);
CRSYNCcode onepiece_magnet_save(const char *filename, magnet_t *magnet);

typedef struct onepiece_t onepiece_t;

typedef enum {
    ONEPIECEOPT_STARTID = 0,
    ONEPIECEOPT_BASEURL,
    ONEPIECEOPT_LOCALAPP,
    ONEPIECEOPT_LOCALRES,
    ONEPIECEOPT_XFER,
} ONEPIECEoption;

typedef enum {
    ONEPIECEINFO_MAGNET = 0,
} ONEPIECEinfo;

CRSYNCcode onepiece_init();
CRSYNCcode onepiece_setopt(ONEPIECEoption opt, void *value);
magnet_t* onepiece_getinfo_magnet();
CRSYNCcode onepiece_perform_query();
CRSYNCcode onepiece_perform_MatchApp();
CRSYNCcode onepiece_perform_PatchApp();
CRSYNCcode onepiece_perform_MatchRes();
CRSYNCcode onepiece_perform_PatchRes();
CRSYNCcode onepiece_perform_updateapp();
CRSYNCcode onepiece_perform_updateres();
void onepiece_cleanup();

#if defined __cplusplus
    }
#endif

#endif // CRSYNC_ONEPIECE_H
