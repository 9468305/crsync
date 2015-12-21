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
    helper_t *elt, *tmp;
    LL_FOREACH_SAFE(h,elt,tmp) {
        LL_DELETE(h,elt);
        free(elt->fileName);
        fileDigest_free(elt->fd);
        diffResult_free(elt->dr);
        free(elt);
    }
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
                LOGI("both-File not exist, new download\n");
                break;
            } else {
                LOGI("dst-File exist, let's compare size\n");
                if((size_t)stDst.st_size == h->fileSize) {
                    LOGI("dst-File size == target-File size; let's compare digest\n");
                    uint8_t digest[CRS_STRONG_DIGEST_SIZE];
                    Digest_CalcStrong_File(srcFullName, digest);
                    if(0 == memcmp(digest, h->fileDigest, CRS_STRONG_DIGEST_SIZE)) {
                        LOGI("Yeah: dst-File digest == target-File digest\n");
                        h->cacheSize = h->fileSize;
                        h->isComplete = 1;
                        break;
                    } else {
                        LOGI("src-File digest != target-File digest\n");
                    }
                } else if((size_t)stDst.st_size < h->fileSize) {
                    LOGI("dst-File size < target-File size, resume download\n");
                    h->cacheSize = stDst.st_size;
                    break;
                } else {
                    LOGI("dst-File size > target-File size\n");
                    LOGI("let's rename dst-File to src-File and diff again\n");
                    Util_filemove(dstFullName, srcFullName);
                }
            }
        }

        LOGI("check src-File digest with target-File\n");
        if((size_t)stSrc.st_size == h->fileSize) {
            LOGI("src-File size == target-File size; let's compare digest\n");
            uint8_t srcDigest[CRS_STRONG_DIGEST_SIZE];
            Digest_CalcStrong_File(srcFullName, srcDigest);
            if(0 == memcmp(srcDigest, h->fileDigest, CRS_STRONG_DIGEST_SIZE)) {
                LOGI("Yeah: src-File digest == target-File digest\n");
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
            LOGI("src-File NotExist\n");
            if(stat(dstFullName, &stDst) != 0 || (size_t)stDst.st_size < h->fileSize) {
                LOGI("dst-File NotExist or < target-File, download it\n");
                code = HTTP_File(url, dstFullName, 5, h->fileName);
                if(stat(dstFullName, &stDst) == 0) {
                    h->cacheSize = stDst.st_size;
                }
                if(code == CRS_OK) {
                    LOGI("dst-File download OK, filemove it\n");
                    Util_filemove(dstFullName, srcFullName);
                } else {
                    LOGE("dst-File download error, Stop\n");
                    break;
                }
            } else {
                LOGE("dst-File size >= target-File size, same or wrong\n");
                LOGI("let's rename dst-File to src-File! check it below\n");
                Util_filemove(dstFullName, srcFullName);
            }
        }

        LOGI("check src-File status\n");
        if(stat(srcFullName, &stSrc) != 0) {
            LOGE("WTF: src-File still not exist!\n");
            code = CRS_BUG;
            break;
        } else {
            LOGI("Yeah: src-File exist, so far so good!\n");
        }

        crs_callback_patch(h->fileName, h->cacheSize, h->isComplete, 1);

        LOGI("let's compare size\n");
        if((size_t)stSrc.st_size == h->fileSize) {
            LOGI("size : src-File == target-File; let's compare digest\n");
            uint8_t srcDigest[CRS_STRONG_DIGEST_SIZE];
            Digest_CalcStrong_File(srcFullName, srcDigest);
            if(0 == memcmp(srcDigest, h->fileDigest, CRS_STRONG_DIGEST_SIZE)) {
                LOGI("digest : src-File == target-File\n");
                code = CRS_OK;
                h->cacheSize = h->fileSize;
                h->isComplete = 1;
                break;
            } else {
                LOGI("digest : src-File != target-File\n");
            }
        } else {
            LOGI("size: src-File != target-File\n");
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
            Util_filemove(dstFullName, srcFullName);
            h->isComplete = 1;
            h->cacheSize = h->fileSize;
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

    crs_callback_patch(h->fileName, h->cacheSize, h->isComplete, 1);

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
        magnet_free(bh->currentMagnet);
        helper_free(bh->currentBulk);
        free(bh);
    }
}
/*
static CRScode perform_magnet(bulkHelper_t *bh, const char *version, magnet_t *m) {
    LOGI("begin\n");
    CRScode code = CRS_OK;
    char *file = Util_strcat(bh->fileDir, version);
    char *fileExt = Util_strcat(file, MAGNET_EXT);
    char *url = Util_strcat(bh->baseUrl, version);
    char *urlExt = Util_strcat(url, MAGNET_EXT);
    do {
        if(0 != Magnet_checkfile(fileExt)) {
            code = HTTP_File(urlExt, fileExt, 1, NULL);
            if(code != CRS_OK) break;
        }
        code = magnet_load(m, fileExt);
    } while(0);

    free(file);
    free(fileExt);
    free(url);
    free(urlExt);
    LOGI("end %d\n", code);
    return code;
}

CRScode bulkHelper_perform_magnet(bulkHelper_t *bh) {
    LOGI("begin\n");
    if(!bh) {
        LOGE("end %d\n", CRS_PARAM_ERROR);
        return CRS_PARAM_ERROR;
    }
    CRScode code = CRS_OK;
    //reset magent
    free(bh->currentMagnet);
    bh->currentMagnet = NULL;

    magnet_t *m = magnet_malloc();
    if(CRS_OK == perform_magnet(bh, bh->currentVersion, m)) {
        bh->currentMagnet = m;
        m = NULL;
    } else {
        magnet_free(m);
        LOGE("end %d currentVersion magnet miss\n", CRS_HTTP_ERROR);
        return CRS_HTTP_ERROR;
    }

    const char *version = bh->currentMagnet->nextVersion;
    while(1) {
        m = magnet_malloc();
        if(CRS_OK == perform_magnet(bh, version, m)) {
            magnet_free(bh->latestMagnet);
            bh->latestMagnet = m;
            m = NULL;
            version = bh->latestMagnet->nextVersion;
        } else {
            magnet_free(m);
            m = NULL;
            break;
        }
    }

    code = CRS_OK;
    if(bh->latestMagnet) {
        free(bh->latestVersion);
        bh->latestVersion = strdup( bh->latestMagnet->currVersion );
    }
    LOGI("end %d\n", code);
    return code;
}
*/

CRScode bulkHelper_set_magnet(bulkHelper_t *bh, const char *magnetString) {
    LOGI("begin\n");
    if(!bh || !magnetString) {
        LOGE("end %d\n", CRS_PARAM_ERROR);
        return CRS_PARAM_ERROR;
    }
    CRScode code = CRS_OK;
    UT_string *str = NULL;
    utstring_new(str);
    utstring_printf(str, "%s", magnetString);
    magnet_t *m = magnet_fromString(&str);
    utstring_free(str);
    free(bh->currentMagnet);
    bh->currentMagnet = m;
    LOGI("end %d\n", code);
    return code;
}

static CRScode perform_diffloop(bulkHelper_t *bh) {
    LOGI("begin\n");
    if(!bh) {
        LOGE("end %d\n", CRS_PARAM_ERROR);
        return CRS_PARAM_ERROR;
    }
    CRScode code = CRS_OK;
    magnet_t *m = bh->currentMagnet;
    if(!m) {
        LOGI("magnet miss, skip this loop\n");
        return code;
    }

    helper_t **bulk = &bh->currentBulk;
    if(!*bulk) {
        sum_t *melt = NULL;
        LL_FOREACH(m->file, melt) {
            helper_t *h = helper_malloc();
            h->fileDir = bh->fileDir;
            h->baseUrl = bh->baseUrl;
            h->fileName = strdup(melt->name);
            h->fileSize = melt->size;
            memcpy(h->fileDigest, melt->digest, CRS_STRONG_DIGEST_SIZE);
            LL_APPEND(*bulk, h);
        }
    }

    helper_t *elt=NULL;
    LL_FOREACH(*bulk,elt) {
        LOGI("%s complete %d\n", elt->fileName, elt->isComplete);
        if(elt->isComplete == 0) {
            code = helper_perform_diff(elt);
            if(code != CRS_OK) break;
            crs_callback_diff(elt->fileName, elt->cacheSize, elt->isComplete);
        }
    }

    LOGI("end %d\n", code);
    return code;
}

CRScode bulkHelper_perform_diff(bulkHelper_t *bh) {
    LOGI("begin\n");
    if(!bh) {
        LOGE("end %d\n", CRS_PARAM_ERROR);
        return CRS_PARAM_ERROR;
    }
    CRScode code = CRS_OK;
    code = perform_diffloop(bh);

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

    helper_t *bulk = bh->currentBulk;
    if(!bulk) {
        LOGE("end bulk is NULL\n");
        return CRS_PARAM_ERROR;
    }

    helper_t *elt=NULL;
    LL_FOREACH(bulk,elt) {
        if(elt->isComplete == 0) {
            code = helper_perform_patch(elt);
            if(code != CRS_OK) break;
        }
    }

    LOGI("end %d\n", code);
    return code;
}
