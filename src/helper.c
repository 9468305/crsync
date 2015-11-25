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
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>

#include "helper.h"
#include "crsync.h"
#include "http.h"
#include "util.h"
#include "log.h"

helper_t* helper_malloc() {
    return calloc(1, sizeof(helper_t));
}
void helper_free(helper_t *h) {
    if(h) {
        free(h->fileName);
        fileDigest_free(h->fd);
        diffResult_free(h->dr);
        free(h);
    }
}

CRScode helper_perform_version() {
    return CRS_BUG;
}

CRScode helper_perform_diff(helper_t *h) {
    LOGI("begin\n");
    if(!h) {
        LOGE("end %d\n", CRS_PARAM_ERROR);
        return CRS_PARAM_ERROR;
    }
    char *srcFullName = Util_strcat(h->fileDir, h->fileName);
    char *digestString = Util_hex_string(h->fileDigest, CRS_STRONG_DIGEST_SIZE);
    char *dstFullName = Util_strcat(h->fileDir, digestString);
    char *url = Util_strcat(h->baseUrl, digestString);
    char *digestUrl = Util_strcat(url, DIGEST_EXT);

    //reset variable
    h->cacheSize = 0;
    h->isComplete = 0;
    fileDigest_free(h->fd);
    h->fd = NULL;
    diffResult_free(h->dr);
    h->dr = NULL;

    CRScode code = CRS_OK;
    do {
        struct stat stSrc;
        struct stat stDst;
        if(stat(srcFullName, &stSrc) != 0) {
            LOGI("src-File not exist\n");
            if(stat(dstFullName, &stDst) != 0) {
                LOGI("dst-File not exist\n");
                LOGI("both-File not exist, skip, new download\n");
                break;
            } else {
                LOGI("dst-File exist\n");
                if((size_t)stDst.st_size < h->fileSize) {
                    LOGI("dst-File size < target-File size, skip, resume download\n");
                    h->cacheSize = stDst.st_size;
                    break;
                } else {
                    LOGI("dst-File size >= target-File size, same or wrong\n");
                    LOGI("let's rename dst-File to src-File, check it below\n");
                    if(0 == rename(dstFullName, srcFullName)) {
                        LOGI("rename OK, check it below\n");
                    }else {
                        LOGE("WTF: rename error %s\n", strerror(errno));
                        code = CRS_FILE_ERROR;
                        break;
                    }
                }
            }
        }

        LOGI("check src-File status again\n");
        if(stat(srcFullName, &stSrc) != 0) {
            LOGE("WTF: src-File still not exist, wired!\n");
            code = CRS_BUG;
            break;
        } else {
            LOGI("Yeah: src-File exist, so far so good!\n");
        }

        LOGI("check src-File digest with target-File\n");
        if((size_t)stSrc.st_size == h->fileSize) {
            LOGI("src-File size == target-File size; let's compare digest\n");
            uint8_t srcDigest[CRS_STRONG_DIGEST_SIZE];
            Digest_CalcStrong_File(srcFullName, srcDigest);
            if(0 == memcmp(srcDigest, h->fileDigest, CRS_STRONG_DIGEST_SIZE)) {
                LOGI("Yeah: src-File digest == target-File digest");
                h->cacheSize = h->fileSize;
                h->isComplete = 1;
                break;
            } else {
                LOGI("src-File digest != target-File digest\n");
            }
        }

        LOGI("let's diff src-File to target-File(fileDigest_t, diffResult_t)\n");
        h->fd = fileDigest_malloc();
        h->dr = diffResult_malloc();
        code = crs_perform_diff(srcFullName, dstFullName, digestUrl, h->fd, h->dr);
        if(code == CRS_OK) {
            h->cacheSize = (h->dr->matchNum + h->dr->cacheNum) * h->fd->blockSize;
        }

    } while(0);

    free(srcFullName);
    free(digestString);
    free(dstFullName);
    free(url);
    free(digestUrl);

    LOGI("end %d\n", code);
    return code;
}

CRScode helper_perform_patch(helper_t *h) {
    LOGI("begin\n");
    if(!h) {
        LOGE("end %d\n", CRS_PARAM_ERROR);
        return CRS_PARAM_ERROR;
    }

    if(h->isComplete > 0 && h->cacheSize == h->fileSize) {
        LOGE("helper_t.isComplete true, cacheSize == fileSize\n");
        LOGE("Should not enter here\n");
        return CRS_OK;
    }

    char *srcFullName = Util_strcat(h->fileDir, h->fileName);
    char *digestString = Util_hex_string(h->fileDigest, CRS_STRONG_DIGEST_SIZE);
    char *dstFullName = Util_strcat(h->fileDir, digestString);
    char *url = Util_strcat(h->baseUrl, digestString);
    char *digestUrl = Util_strcat(url, DIGEST_EXT);

    CRScode code = CRS_OK;
    do {
        struct stat stSrc;
        struct stat stDst;
        if(stat(srcFullName, &stSrc) != 0) {
            LOGI("src-File not exist\n");
            if(stat(dstFullName, &stDst) != 0) {
                LOGI("dst-File not exist\n");
                LOGI("both-File not exist, new download\n");
                code = HTTP_File(url, dstFullName, 5, NULL);
                if(code == CRS_OK) {
                    LOGI("dst-File new download OK, let's rename it\n");
                    if( 0 == rename(dstFullName, srcFullName) ) {
                        LOGI("rename OK, let's check digest\n");
                    } else {
                        LOGE("WTF: rename error %s\n", strerror(errno));
                        code = CRS_FILE_ERROR;
                        break;
                    }
                } else {
                    LOGE("dst-File new download error, stop\n");
                    break;
                }
            } else {
                LOGI("dst-File exist\n");
                if((size_t)stDst.st_size < h->fileSize) {
                    LOGI("dst-File size < target-File size, resume download\n");
                    code = HTTP_File(url, dstFullName, 5, NULL);
                    if(code == CRS_OK) {
                        LOGI("dst-File resume download OK, let's rename it\n");
                        if( 0 != rename(dstFullName, srcFullName) ) {
                            LOGI("rename OK, let's check digest\n");
                        } else {
                            LOGE("WTF: rename error %s\n", strerror(errno));
                            code = CRS_FILE_ERROR;
                            break;
                        }
                    } else {
                        LOGE("dst-File resume download error, Stop\n");
                        break;
                    }
                } else {
                    LOGE("dst-File size >= target-File size, same or wrong\n");
                    LOGE("This should not happen!\n");
                    LOGI("let's rename dst-File to src-File! check it below\n");
                    if(0 == rename(dstFullName, srcFullName)) {
                        LOGI("rename OK, let's check digest\n");
                    } else {
                        LOGE("WTF: rename error %s\n", strerror(errno));
                        code = CRS_FILE_ERROR;
                        break;
                    }
                }
            }
        }

        LOGI("check src-File status\n");
        if(stat(srcFullName, &stSrc) != 0) {
            LOGE("WTF: src-File still not exist, wired!\n");
            code = CRS_BUG;
            break;
        } else {
            LOGI("Yeah: src-File exist, so far so good!\n");
        }

        LOGI("check src-File digest with target-File\n");
        if((size_t)stSrc.st_size == h->fileSize) {
            LOGI("src-File size == target-File size; let's compare digest\n");
            uint8_t srcDigest[CRS_STRONG_DIGEST_SIZE];
            Digest_CalcStrong_File(srcFullName, srcDigest);
            if(0 == memcmp(srcDigest, h->fileDigest, CRS_STRONG_DIGEST_SIZE)) {
                LOGI("Yeah: src-File digest == target-File digest");
                code = CRS_OK;
                h->cacheSize = h->fileSize;
                h->isComplete = 1;
                break;
            } else {
                LOGI("src-File digest != target-File digest\n");
            }
        }

        LOGI("check fileDigest_t and diffResult_t to keep safe\n");
        if(!h->fd || !h->dr) {
            LOGE("fileDigest_t or diffResult_t NULL, should not happend\n");
            LOGE("Maybe HTTP_File failed for check digest\n");
            LOGE("Reset and Run diff again to keep safe\n");

            fileDigest_free(h->fd);
            diffResult_free(h->dr);

            h->fd = fileDigest_malloc();
            h->dr = diffResult_malloc();

            code = crs_perform_diff(srcFullName, dstFullName, digestUrl, h->fd, h->dr);
            if(code == CRS_OK) {
                h->cacheSize = (h->dr->matchNum + h->dr->cacheNum) * h->fd->blockSize;
            } else {
                LOGE("What can I do here? Nothing\n");
                break;
            }
        }

        code = crs_perform_patch(srcFullName, dstFullName, url, h->fd, h->dr);
        if(code == CRS_OK) {
            LOGI("Patch OK, crs_perform_patch make sure fileDigest right\n");
            LOGI("let's remove(delete) src-File\n");
            if(0 == remove(srcFullName)) {
                LOGI("remove(delete) src-File OK\n");
            } else {
                LOGE("WTF: remove(delete) src-File error %s", strerror(errno));
            }
            LOGI("let's rename dst-File to src-File\n");
            if(0 == rename(dstFullName, srcFullName)) {
                LOGI("rename OK, patch complete\n");
                h->isComplete = 1;
                h->cacheSize = h->fileSize;
            } else {
                LOGE("WTF: rename error %s\n", strerror(errno));
                code = CRS_FILE_ERROR;
            }
            break;
        }
    } while(0);

    free(srcFullName);
    free(digestString);
    free(dstFullName);
    free(url);
    free(digestUrl);

    fileDigest_free(h->fd);
    h->fd = NULL;
    diffResult_free(h->dr);
    h->dr = NULL;

    LOGI("end %d\n", code);
    return code;
}

bulkHelper_t * bulkHelper_malloc() {
    bulkHelper_t* bh = calloc(1, sizeof(bulkHelper_t));
    return bh;
}

void bulkHelper_free(bulkHelper_t *bh) {
    if(bh) {
        free(bh->fileDir);
        free(bh->baseUrl);
        for(unsigned int i=0; i<bh->bulkSize; ++i) {
            helper_free(&bh->bulk[i]);
        }
        free(bh);
    }
}

CRScode bulkHelper_perform_diff(bulkHelper_t *bh) {
    LOGI("begin\n");
    if(!bh) {
        LOGE("end %d\n", CRS_PARAM_ERROR);
        return CRS_PARAM_ERROR;
    }
    CRScode code = CRS_OK;

    for(unsigned int i=0; i< bh->bulkSize; ++i) {
        code = helper_perform_diff(&bh->bulk[i]);
        if(code != CRS_OK) break;
    }

    LOGI("end %d\n", code);
    return code;
}

CRScode bulkHelper_perform_patch(bulkHelper_t *bh) {
    LOGI("begin\n");
    if(!bh) {
        LOGE("end %d\n", CRS_PARAM_ERROR);
        return CRS_PARAM_ERROR;
    }
    CRScode code = CRS_OK;

    for(unsigned int i=0; i< bh->bulkSize; ++i) {
        code = helper_perform_patch(&bh->bulk[i]);
        if(code != CRS_OK) break;
    }

    LOGI("end %d\n", code);
    return code;
}
