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
#include <unistd.h>
#include <sys/stat.h>
#include "onepiece.h"
#include "log.h"

static const int MAGNET_CURL_RETRY = 2;
static const unsigned int MAGNET_SLEEP_RETRY = 1;
static const int MAX_CURL_RETRY = 5;
static const unsigned int SLEEP_CURL_RETRY = 5;

CRSYNCcode onepiece_magnet_load(const char *magnetFilename, magnet_t *magnet) {
    LOGI("onepiece_magnet_load\n");
    CRSYNCcode code = CRSYNCE_OK;
    char *resname = NULL, *reshash = NULL;
    tpl_node *tn = tpl_map(MAGNET_TPLMAP_FORMAT,
                 &magnet->curr_id,
                 &magnet->next_id,
                 &magnet->apphash,
                 &resname,
                 &reshash);
    if (0 == tpl_load(tn, TPL_FILE, magnetFilename) ) {
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

typedef struct onepiece_t {
    magnet_t *magnet;
    CURL *curl_handle;
    crsync_handle_t *crsync_handle;
    char *start_id;
    char *baseUrl;
    char *localApp;
    char *localRes;
    xfer_callback *xfer;
} onepiece_t;

static onepiece_t *onepiece = NULL;

CRSYNCcode onepiece_init() {
    LOGI("onepiece_init\n");
    CRSYNCcode code = CRSYNCE_OK;
    do {
        if(onepiece) {
            code = CRSYNCE_FAILED_INIT;
            break;
        }
        code = crsync_global_init();
        if(CRSYNCE_OK != code) break;
        onepiece = calloc(1, sizeof(onepiece_t));
        onepiece->xfer = xfer_default;
        onepiece_magnet_new(onepiece->magnet);
        onepiece->curl_handle = curl_easy_init();
        code = (NULL == onepiece->curl_handle) ? CRSYNCE_FAILED_INIT : CRSYNCE_OK;
        if(CRSYNCE_OK != code) break;
        onepiece->crsync_handle = crsync_easy_init();
        code = (NULL == onepiece->crsync_handle) ? CRSYNCE_FAILED_INIT : CRSYNCE_OK;
        if(CRSYNCE_OK != code) break;
    } while(0);

    if(CRSYNCE_OK != code && onepiece) {
        if(onepiece->crsync_handle) {
            crsync_easy_cleanup(onepiece->crsync_handle);
        }
        if(onepiece->curl_handle) {
            curl_easy_cleanup(onepiece->curl_handle);
        }
        if(onepiece->magnet) {
            onepiece_magnet_free(onepiece->magnet);
        }
        free(onepiece);
        onepiece = NULL;
    }

    LOGI("onepiece_init code = %d\n", code);
    return code;
}

CRSYNCcode onepiece_setopt(ONEPIECEoption opt, void *value) {
    LOGI("onepiece_setopt %d\n", opt);
    CRSYNCcode code = CRSYNCE_OK;
    do {
        if(!onepiece) {
            code = CRSYNCE_INVALID_OPT;
            break;
        }
        switch(opt) {
        case ONEPIECEOPT_STARTID:
            if(onepiece->start_id) {
                free(onepiece->start_id);
            }
            onepiece->start_id = strdup( (const char*)value );
            break;
        case ONEPIECEOPT_BASEURL:
            if(onepiece->baseUrl) {
                free(onepiece->baseUrl);
            }
            onepiece->baseUrl = strdup( (const char*)value );
            break;
        case ONEPIECEOPT_LOCALAPP:
            if(onepiece->localApp) {
                free(onepiece->localApp);
            }
            onepiece->localApp = strdup( (const char*)value );
            break;
        case ONEPIECEOPT_LOCALRES:
            if(onepiece->localRes) {
                free(onepiece->localRes);
            }
            onepiece->localRes = strdup( (const char*)value );
            break;
        case ONEPIECEOPT_XFER:
            onepiece->xfer = (xfer_callback*)value;
            break;
        default:
            LOGE("Unknown opt\n");
            code = CRSYNCE_INVALID_OPT;
            break;
        }
    } while(0);
    LOGI("onepiece_setopt code = %d\n", code);
    return code;
}

static CRSYNCcode onepiece_checkopt() {
    if( onepiece &&
        onepiece->magnet &&
        onepiece->curl_handle &&
        onepiece->crsync_handle &&
        onepiece->start_id &&
        onepiece->baseUrl &&
        onepiece->localApp &&
        onepiece->localRes ) {
        return CRSYNCE_OK;
    }
    return CRSYNCE_INVALID_OPT;
}

static CRSYNCcode onepiece_magnet_curl(const char *id, magnet_t *magnet) {
    LOGI("onepiece_magnet_curl %s\n", id);
    CRSYNCcode code = CRSYNCE_CURL_ERROR;
    UT_string *magnetUrl = get_full_string(onepiece->baseUrl, id, MAGNET_SUFFIX);
    UT_string *magnetFilename = get_full_string(onepiece->localRes, id, MAGNET_SUFFIX);

    int retry = 0;
    do {
        FILE *magnetFile = fopen(utstring_body(magnetFilename), "wb");
        if(magnetFile) {
            crsync_curl_setopt(onepiece->curl_handle);
            curl_easy_setopt(onepiece->curl_handle, CURLOPT_URL, utstring_body(magnetUrl));
            curl_easy_setopt(onepiece->curl_handle, CURLOPT_HTTPGET, 1L);
            curl_easy_setopt(onepiece->curl_handle, CURLOPT_WRITEFUNCTION, NULL);
            curl_easy_setopt(onepiece->curl_handle, CURLOPT_WRITEDATA, (void *)magnetFile);
            curl_easy_setopt(onepiece->curl_handle, CURLOPT_NOPROGRESS, 1L);
            CURLcode curlcode = curl_easy_perform(onepiece->curl_handle);
            fclose(magnetFile);
            LOGI("onepiece_magnet_curl curlcode = %d\n", curlcode);
            if( CURLE_OK == curlcode) {
                if( CRSYNCE_OK == crsync_tplfile_check(utstring_body(magnetFilename), MAGNET_TPLMAP_FORMAT) ) {
                    LOGI("onepiece_magnet_curl gotfile\n");
                    code = CRSYNCE_OK;
                    break;
                }
            }
        } else {
            code = CRSYNCE_FILE_ERROR;
            break;
        }
        sleep(MAGNET_SLEEP_RETRY * (++retry));
    } while(retry < MAGNET_CURL_RETRY);
    curl_easy_reset(onepiece->curl_handle);

    if(CRSYNCE_OK == code) {
        code = onepiece_magnet_load(utstring_body(magnetFilename), magnet);
    }

    utstring_free(magnetUrl);
    utstring_free(magnetFilename);
    LOGI("onepiece_magnet_curl code = %d\n", code);
    return code;
}

static CRSYNCcode onepiece_magnet_query(const char *id, magnet_t *magnet) {
    LOGI("onepiece_magnet_query id = %s\n", id);
    CRSYNCcode code;
    UT_string *magnetFilename = get_full_string(onepiece->localRes, id, MAGNET_SUFFIX);
    if(CRSYNCE_OK == crsync_tplfile_check(utstring_body(magnetFilename), MAGNET_TPLMAP_FORMAT)) {
        code = onepiece_magnet_load(utstring_body(magnetFilename), magnet);
    } else {
        code = onepiece_magnet_curl(id, magnet);
    }
    utstring_free(magnetFilename);
    LOGI("onepiece_magnet_query code = %d", code);
    return code;
}

UT_string* onepiece_getinfo(ONEPIECEinfo info) {
    LOGI("onepiece_getinfo %d\n", info);
    UT_string *result = NULL;
    utstring_new(result);
    switch (info) {
    case ONEPIECEINFO_QUERY:
        if(onepiece && onepiece->magnet) {
            utstring_printf(result, "%s;", onepiece->magnet->curr_id);
            utstring_printf(result, "%s;", onepiece->magnet->next_id);
            utstring_printf(result, "%s;", onepiece->magnet->apphash);
            char **p = NULL, **q = NULL;
            while ( (p=(char**)utarray_next(onepiece->magnet->resname,p)) && (q=(char**)utarray_next(onepiece->magnet->reshash,q)) ) {
                utstring_printf(result, "%s;", *p);
                utstring_printf(result, "%s;", *q);
            }
        }
        break;
    default:
        LOGE("Unknown info\n");
        break;
    }
    LOGI("onepiece_getinfo result : %s\n", utstring_body(result));
    return result;
}

CRSYNCcode onepiece_perform_query() {
    LOGI("onepiece_perform_query\n");
    CRSYNCcode code = onepiece_checkopt();
    if(CRSYNCE_OK != code) {
        LOGE("onepiece_perform_query code = %d\n", code);
        return code;
    }
    const char *id = onepiece->start_id;
    magnet_t *magnet = NULL;
    onepiece_magnet_new(magnet);
    while( CRSYNCE_OK == onepiece_magnet_query(id, magnet) ) {
        if(onepiece->magnet) {
            onepiece_magnet_free(onepiece->magnet);
        }
        onepiece->magnet = magnet;
        onepiece_magnet_new(magnet);
        id = onepiece->magnet->next_id;
    }
    code = (NULL == onepiece->magnet->curr_id) ? CRSYNCE_CURL_ERROR : CRSYNCE_OK;
    LOGI("onepiece_perform_query code = %d\n", code);
    return code;
}

CRSYNCcode onepiece_perform_updateapp() {
    LOGI("onepiece_perform_updateapp\n");
    CRSYNCcode code = onepiece_checkopt();
    if(CRSYNCE_OK != code) {
        LOGE("onepiece_perform_updateapp code = %d\n", code);
        return code;
    }

    do {
        //needn't compare old file to new file, since magnet said has new version.
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_OUTPUTDIR, onepiece->localRes);
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_FILE, onepiece->localApp);
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_HASH, onepiece->magnet->apphash);
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_BASEURL, onepiece->baseUrl);
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_XFER, (void*)onepiece->xfer);

        code = crsync_easy_perform_match(onepiece->crsync_handle);
        if(CRSYNCE_OK != code) break;
        code = crsync_easy_perform_patch(onepiece->crsync_handle);
        if(CRSYNCE_OK != code) break;
        onepiece->xfer(onepiece->magnet->apphash, 100);
    } while(0);

    LOGI("onepiece_perform_updateapp code = %d", code);
    return code;
}

static curl_off_t localFileResumeSize = 0L;

static int onepiece_updateres_curl_progress(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    const char *hash = (const char*)clientp;
    (void)ultotal;
    (void)ulnow;
    // dltotal dlnow only for current download process, exclude local resume size, so add it here
    int percent = (dltotal > 0) ? ( (dlnow + localFileResumeSize) * 100 / dltotal) : 0;
    onepiece->xfer(hash, percent);
    return 0;
}

static CRSYNCcode onepiece_updateres_curl(const char *resname, const char *reshash) {
    LOGI("onepiece_updateres_curl %s\n", resname);
    CRSYNCcode code = CRSYNCE_CURL_ERROR;
    UT_string *url = get_full_string(onepiece->baseUrl, reshash, NULL);
    UT_string *filename = get_full_string(onepiece->localRes, resname, NULL);

    int retry = 0;
    do {
        localFileResumeSize = 0L;
        struct stat file_info;
        if(stat(utstring_body(filename), &file_info) == 0) {
            localFileResumeSize = file_info.st_size;
        }
        FILE *file = fopen(utstring_body(filename), "ab+");
        if(file) {
            crsync_curl_setopt(onepiece->curl_handle);
            curl_easy_setopt(onepiece->curl_handle, CURLOPT_URL, utstring_body(url));
            curl_easy_setopt(onepiece->curl_handle, CURLOPT_HTTPGET, 1L);
            curl_easy_setopt(onepiece->curl_handle, CURLOPT_WRITEFUNCTION, NULL);
            curl_easy_setopt(onepiece->curl_handle, CURLOPT_WRITEDATA, (void *)file);
            curl_easy_setopt(onepiece->curl_handle, CURLOPT_NOPROGRESS, 0L);
            curl_easy_setopt(onepiece->curl_handle, CURLOPT_XFERINFOFUNCTION, onepiece_updateres_curl_progress);
            curl_easy_setopt(onepiece->curl_handle, CURLOPT_XFERINFODATA, (void*)reshash);
            curl_easy_setopt(onepiece->curl_handle, CURLOPT_RESUME_FROM_LARGE, localFileResumeSize);

            CURLcode curlcode = curl_easy_perform(onepiece->curl_handle);
            fclose(file);
            LOGI("onepiece_updateres_curl curlcode = %d\n", curlcode);
            if( CURLE_OK == curlcode) {
                code = CRSYNCE_OK;
                break;
            } else if(CURLE_WRITE_ERROR == curlcode) {
                code = CRSYNCE_FILE_ERROR;
                break;
            } else {
                code = CRSYNCE_CURL_ERROR;
            }
        } else {
            code = CRSYNCE_FILE_ERROR;
            break;
        }
        sleep(SLEEP_CURL_RETRY * (++retry));
    } while(retry < MAX_CURL_RETRY);
    curl_easy_reset(onepiece->curl_handle);

    utstring_free(url);
    utstring_free(filename);
    LOGI("onepiece_updateres_curl code = %d\n", code);
    return code;
}

CRSYNCcode onepiece_perform_updateres() {
    LOGI("onepiece_perform_updateres\n");
    CRSYNCcode code = onepiece_checkopt();
    if(CRSYNCE_OK != code) {
        LOGE("onepiece_perform_updateres code = %d\n", code);
        return code;
    }

    uint8_t strong[STRONG_SUM_SIZE];
    UT_string *hash = NULL;
    utstring_new(hash);
    char **p = NULL, **q = NULL;
    while ( (p=(char**)utarray_next(onepiece->magnet->resname,p)) && (q=(char**)utarray_next(onepiece->magnet->reshash,q)) ) {
        LOGI("updateres %s %s\n", *p, *q);
        UT_string *localfile = get_full_string( onepiece->localRes, *p, NULL);
        //0. if old file not exist, curl download directly, then re-calc hash
        if(-1 == rsum_strong_file(utstring_body(localfile), strong)) {
            code = onepiece_updateres_curl(*p, *q);
            if(CRSYNCE_OK != code) break;
            rsum_strong_file(utstring_body(localfile), strong);
        }
        //1. compare old file to new file, by strong hash blake2
        utstring_clear(hash);
        for(int j=0; j < STRONG_SUM_SIZE; j++) {
            utstring_printf(hash, "%02x", strong[j] );
        }
        LOGI("localres hash = %s\n", utstring_body(hash));
        //2. skip update if hash is same
        if(0 == strncmp(utstring_body(hash), *q, strlen(*q))) {
            onepiece->xfer(*q, 100);
            utstring_free(localfile);
            continue;
        }
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_OUTPUTDIR, onepiece->localRes);
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_FILE, utstring_body(localfile));
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_HASH, *q);
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_BASEURL, onepiece->baseUrl);
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_XFER, (void*)onepiece->xfer);

        utstring_free(localfile);
        code = crsync_easy_perform_match(onepiece->crsync_handle);
        if(CRSYNCE_OK != code) break;
        code = crsync_easy_perform_patch(onepiece->crsync_handle);
        if(CRSYNCE_OK != code) break;
        UT_string *oldfile = get_full_string( onepiece->localRes, *p, NULL);
        UT_string *newfile = get_full_string( onepiece->localRes, *q, NULL );
        remove( utstring_body(oldfile) );
        code = (0 == rename(utstring_body(newfile), utstring_body(oldfile))) ? CRSYNCE_OK : CRSYNCE_FILE_ERROR;
        utstring_free(oldfile);
        utstring_free(newfile);
        if(CRSYNCE_OK != code) break;
        onepiece->xfer(*q, 100);
    }

    utstring_free(hash);
    LOGI("onepiece_perform_updateres code = %d\n", code);
    return code;
}

void onepiece_cleanup() {
    LOGI("onepiece_cleanup\n");
    if(!onepiece){
        return;
    }
    onepiece_magnet_free(onepiece->magnet);
    curl_easy_cleanup(onepiece->curl_handle);
    crsync_easy_cleanup(onepiece->crsync_handle);
    free(onepiece->start_id);
    free(onepiece->baseUrl);
    free(onepiece->localApp);
    free(onepiece->localRes);
    free(onepiece);
    onepiece = NULL;

    crsync_global_cleanup();
}
