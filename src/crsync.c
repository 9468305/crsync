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
#include <stdio.h>
#if ( defined __CYGWIN__ || defined __MINGW32__ || defined _WIN32 )
#   include "win/mman.h"   /* mmap */
#else
#   include <sys/mman.h>   /* mmap */
#endif

#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <omp.h>

#include "unistd-cross.h"
#include "crsync.h"
#include "http.h"
#include "log.h"
#include "util.h"

CRScode crs_perform_digest(const char *srcFilename, const char *dstFilename, const uint32_t blockSize) {
    LOGI("begin\n");
    if(srcFilename == NULL || dstFilename == NULL) {
        LOGE("end %d\n", CRS_PARAM_ERROR);
        return CRS_PARAM_ERROR;
    }

    fileDigest_t *fd = fileDigest_malloc();
    CRScode code = CRS_OK;
    do {
        code = Digest_Perform(srcFilename, blockSize, fd);
        if(code != CRS_OK) break;
        code = Digest_Save(dstFilename, fd);
    } while(0);

    fileDigest_free(fd);

    LOGI("end %d\n", code);
    return code;
}

CRScode crs_perform_diff(const char *srcFilename, const char *dstFilename, const char *digestUrl,
                         fileDigest_t *fd, diffResult_t *dr) {
    LOGI("begin\n");

    if(!srcFilename || !dstFilename || !digestUrl || !fd || !dr) {
        LOGE("end %d\n", CRS_PARAM_ERROR);
        return CRS_PARAM_ERROR;
    }

    CRScode code = CRS_OK;
    char *digestFilename = Util_strcat(dstFilename, DIGEST_EXT);
    LOGI("digestFilename = %s\n", digestFilename);

    do {
        if(0 != Digest_checkfile(digestFilename)) {
            code = HTTP_File(digestUrl, digestFilename, 1, NULL);
            if(code != CRS_OK) break;
        }

        code = Digest_Load(digestFilename, fd);
        if(code != CRS_OK) break;
        code = Diff_perform(srcFilename, dstFilename, fd, dr);
    } while(0);

    free(digestFilename);
    fileDigest_dump(fd);
    diffResult_dump(dr);
    LOGI("end %d\n", code);
    return code;
}

CRScode crs_perform_patch(const char *srcFilename, const char *dstFilename, const char *url,
                          const fileDigest_t *fd, const diffResult_t *dr) {
    LOGI("begin\n");
    CRScode code = CRS_OK;
    code = Patch_perform(srcFilename, dstFilename, url, fd, dr);
    LOGI("end %d\n", code);
    return code;
}

CRScode crs_perform_update(const char *srcFilename, const char *dstFilename, const char *digestUrl, const char *url) {
    LOGI("begin\n");

    if(!srcFilename || !dstFilename || !url) {
        LOGE("end %d\n", CRS_PARAM_ERROR);
        return CRS_PARAM_ERROR;
    }

    CRScode code = CRS_OK;

    fileDigest_t *fd = fileDigest_malloc();
    diffResult_t *dr = diffResult_malloc();
    do {
        code = crs_perform_diff(srcFilename, dstFilename, digestUrl, fd, dr);
        if(code != CRS_OK) break;
        code = crs_perform_patch(srcFilename, dstFilename, url, fd, dr);
    } while(0);

    fileDigest_free(fd);
    diffResult_free(dr);

    LOGI("end %d\n", code);
    return code;
}
