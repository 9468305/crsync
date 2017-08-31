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
#ifndef CRSYNC_H
#define CRSYNC_H

#if defined __cplusplus
extern "C" {
#endif

#include "global.h"
#include "crsyncver.h"         /* crsync version defines   */
#include "digest.h"
#include "diff.h"
#include "patch.h"
#include "uthash.h"
#include "utlist.h"
#include "utstring.h"
#include "tpl.h"
#include "curl.h"

CRScode crs_perform_digest  (const char *srcFilename, const char *dstFilename, const uint32_t blocksize);

CRScode crs_perform_diff    (const char *srcFilename, const char *dstFilename, const char *digestUrl,
                            fileDigest_t *fd, diffResult_t *dr);

CRScode crs_perform_patch   (const char *srcFilename, const char *dstFilename, const char *url,
                            const fileDigest_t *fd, const diffResult_t *dr);

CRScode crs_perform_update  (const char *srcFilename, const char *dstFilename, const char *digestUrl, const char *url);

#if defined __cplusplus
}
#endif

#endif // CRSYNC_H
