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
#ifndef CRS_MAGNET_H
#define CRS_MAGNET_H

#include "global.h"
#include "utstring.h"

extern const char *MAGNET_EXT;

typedef struct sum_t {
    char            *name;
    unsigned int    size;
    unsigned char   digest[CRS_STRONG_DIGEST_SIZE];
    struct sum_t    *next; //utlist(single-link)
} sum_t;

sum_t*  sum_malloc();
void    sum_free(sum_t *s);

typedef struct magnet_t {
    char    *currVersion;
    char    *nextVersion;
    sum_t   *file; //utlist(single-link)
} magnet_t;

magnet_t*   magnet_malloc();
void        magnet_free(magnet_t *m);

void        magnet_toString(magnet_t *m, UT_string **str);
magnet_t*   magnet_fromString(UT_string **str);

CRScode     magnet_load(magnet_t *m, const char *file);
CRScode     magnet_save(magnet_t *m, const char *file);

int Magnet_checkfile(const char *filename);

#endif // CRS_MAGNET_H

