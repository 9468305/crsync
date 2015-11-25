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
#include <fcntl.h>
#include <sys/stat.h>
#include "onepiece.h"
#include "log.h"

static const int MAGNET_CURL_RETRY = 2;
static const unsigned int MAGNET_SLEEP_RETRY = 1;
static const int MAX_CURL_RETRY = 5;
static const unsigned int SLEEP_CURL_RETRY = 5;

static int isNeverUpdate = 0; // auto update = 0; never update = 1;

void res_free(res_t *res) {
    res_t *elt, *tmp;
    LL_FOREACH_SAFE(res,elt,tmp) {
        LL_DELETE(res,elt);
        free(elt->name);
        free(elt->hash);
        free(elt);
    }
}

oldmagnet_t* onepiece_magnet_malloc() {
    oldmagnet_t *m=calloc(1, sizeof(oldmagnet_t));
    return m;
}

void onepiece_magnet_free(oldmagnet_t* m) {
    free(m->curr_id);
    free(m->next_id);
    res_free(m->app);
    res_free(m->res_list);
    free(m);
}

CRSYNCcode onepiece_magnet_load(const char *filename, oldmagnet_t *magnet) {
    LOGI("onepiece_magnet_load\n");
    CRSYNCcode code = CRSYNCE_OK;
    char *apphash = NULL, *resname = NULL, *reshash = NULL;
    tpl_node *tn = tpl_map(MAGNET_TPLMAP_FORMAT,
                 &magnet->curr_id,
                 &magnet->next_id,
                 &apphash,
                 &resname,
                 &reshash);
    if (0 == tpl_load(tn, TPL_FILE, filename) ) {
        tpl_unpack(tn, 0);
        res_t *app = calloc(1, sizeof(res_t));
        app->hash = strdup(apphash);
        LL_APPEND(magnet->app, app);
        while (tpl_unpack(tn, 1) > 0) {
            res_t *res = calloc(1, sizeof(res_t));
            res->name = resname;
            res->hash = reshash;
            LL_APPEND(magnet->res_list, res);
        }
    } else {
        code = CRSYNCE_FILE_ERROR;
    }
    tpl_free(tn);
    LOGI("onepiece_magnet_load code = %d\n", code);
    return code;
}

CRSYNCcode onepiece_magnet_save(const char *filename, oldmagnet_t *magnet) {
    char *resname=NULL, *reshash=NULL;
    tpl_node *tn = tpl_map(MAGNET_TPLMAP_FORMAT,
                 &magnet->curr_id,
                 &magnet->next_id,
                 &magnet->app->hash,
                 &resname,
                 &reshash);
    tpl_pack(tn, 0);
    res_t *elt=NULL;
    LL_FOREACH(magnet->res_list,elt) {
        resname = elt->name;
        reshash = elt->hash;
        tpl_pack(tn, 1);
    }
    int result = tpl_dump(tn, TPL_FILE, filename);
    tpl_free(tn);
    return (-1 == result) ? CRSYNCE_FILE_ERROR : CRSYNCE_OK;
}

typedef struct onepiece_t {
    oldmagnet_t *magnet;
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
        isNeverUpdate = 0;
        onepiece->xfer = xfer_default;
        onepiece->magnet = onepiece_magnet_malloc();
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
        case ONEPIECEOPT_NEVERUPDATE:
            if(0 == strncmp((const char*)value, "1", 1)) {
                isNeverUpdate = 1;
            } else {
                isNeverUpdate = 0;
            }
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

static CRSYNCcode onepiece_magnet_curl(const char *id, oldmagnet_t *magnet) {
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

static CRSYNCcode onepiece_magnet_query(const char *id, oldmagnet_t *magnet) {
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

oldmagnet_t* onepiece_getinfo_magnet() {
    if(onepiece) {
        return onepiece->magnet;
    } else {
        return NULL;
    }
}

CRSYNCcode onepiece_perform_query() {
    LOGI("onepiece_perform_query\n");
    CRSYNCcode code = onepiece_checkopt();
    if(CRSYNCE_OK != code) {
        LOGE("onepiece_perform_query code = %d\n", code);
        return code;
    }
    const char *id = onepiece->start_id;
    oldmagnet_t *magnet = onepiece_magnet_malloc();
    
    //temp implement from boss!
    if(isNeverUpdate == 1) {
        if(CRSYNCE_OK == onepiece_magnet_query(id, magnet)) {
            onepiece_magnet_free(onepiece->magnet);
            onepiece->magnet = magnet;
        }
        code = (NULL == onepiece->magnet->curr_id) ? CRSYNCE_CURL_ERROR : CRSYNCE_OK;
        LOGI("onepiece_perform_query code = %d\n", code);
        return code;
    }

    while( CRSYNCE_OK == onepiece_magnet_query(id, magnet) ) {
        onepiece_magnet_free(onepiece->magnet);
        onepiece->magnet = magnet;
        magnet = onepiece_magnet_malloc();
        id = onepiece->magnet->next_id;
    }
    code = (NULL == onepiece->magnet->curr_id) ? CRSYNCE_CURL_ERROR : CRSYNCE_OK;
    onepiece_magnet_free(magnet);
    LOGI("onepiece_perform_query code = %d\n", code);
    log_dump();
    return code;
}

CRSYNCcode onepiece_perform_MatchApp() {
    LOGI("onepiece_perform_MatchApp\n");
    CRSYNCcode code = onepiece_checkopt();
    if(CRSYNCE_OK != code) {
        LOGE("onepiece_perform_MatchApp code = %d\n", code);
        return code;
    }
    do {
        //needn't compare old file to new file, since magnet said has new version.
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_OUTPUTDIR, onepiece->localRes);
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_FILE, onepiece->localApp);
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_HASH, onepiece->magnet->app->hash);
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_BASEURL, onepiece->baseUrl);
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_XFER, (void*)onepiece->xfer);

        code = crsync_easy_perform_match(onepiece->crsync_handle);
        if(CRSYNCE_OK == code) {
            onepiece->magnet->app->size = onepiece->crsync_handle->meta->file_sz;
            onepiece->magnet->app->diff_size = onepiece->crsync_handle->meta->file_sz - onepiece->crsync_handle->meta->same_count * onepiece->crsync_handle->meta->block_sz;
        }
    } while(0);

    LOGI("onepiece_perform_MatchApp code = %d", code);
    log_dump();
    return code;
}

CRSYNCcode onepiece_perform_PatchApp() {
    LOGI("onepiece_perform_PatchApp\n");
    CRSYNCcode code = onepiece_checkopt();
    if(CRSYNCE_OK != code) {
        LOGE("onepiece_perform_updateapp code = %d\n", code);
        return code;
    }
    do {
        //needn't compare old file to new file, since magnet said has new version.
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_OUTPUTDIR, onepiece->localRes);
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_FILE, onepiece->localApp);
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_HASH, onepiece->magnet->app->hash);
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_BASEURL, onepiece->baseUrl);
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_XFER, (void*)onepiece->xfer);

        //call match to update msums to crsync_handle
        code = crsync_easy_perform_match(onepiece->crsync_handle);
        if(CRSYNCE_OK != code) break;
        code = crsync_easy_perform_patch(onepiece->crsync_handle);
        if(CRSYNCE_OK != code) break;
        onepiece->xfer(onepiece->magnet->app->hash, 100, 0);
    } while(0);

    LOGI("onepiece_perform_PatchApp code = %d", code);
    log_dump();
    return code;
}

CRSYNCcode onepiece_perform_MatchRes() {
    LOGI("onepiece_perform_MatchRes\n");
    CRSYNCcode code = onepiece_checkopt();
    if(CRSYNCE_OK != code) {
        LOGE("onepiece_perform_MatchRes code = %d\n", code);
        return code;
    }

    uint8_t strong[STRONG_SUM_SIZE];
    UT_string *hash = NULL;
    utstring_new(hash);

    res_t *elt=NULL;
    LL_FOREACH(onepiece->magnet->res_list, elt) {
        LOGI("MatchRes %s %s %d\n", elt->name, elt->hash, elt->size);
        utstring_clear(hash);
        UT_string *localfile = get_full_string( onepiece->localRes, elt->name, NULL);
        //0. if old file not exist, skip match
        if( -1 == access( utstring_body(localfile), F_OK ) ) {
            //do nothing
        } else {
            UT_string* file_hash = get_full_string(onepiece->localRes, elt->name, elt->hash);
            if(0 == access(utstring_body(file_hash), F_OK)) {
                utstring_printf(hash, "%s", elt->hash);
            } else {
                //calc hash & string
                rsum_strong_file(utstring_body(localfile), strong);
                for(int j=0; j < STRONG_SUM_SIZE; j++) {
                    utstring_printf(hash, "%02x", strong[j] );
                }
                //create localCRC namehash
                UT_string *localfile_hash = get_full_string(onepiece->localRes, elt->name, utstring_body(hash));
                creat(utstring_body(localfile_hash), 700);
                utstring_free(localfile_hash);
            }
            utstring_free(file_hash);
        }
        LOGI("localres hash = %s\n", utstring_body(hash));
        //2. skip update if hash is same
        if(0 == strncmp(utstring_body(hash), elt->hash, strlen(elt->hash))) {
            elt->size = 100; //TODO: here we do not know rsum meta file size, should add file size to magnet struct.
            //TODO: here now only happens when development, since user should have hashcache before.
            elt->diff_size = 0;
            utstring_free(localfile);
            continue;
        }
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_OUTPUTDIR, onepiece->localRes);
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_FILE, utstring_body(localfile));
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_HASH, elt->hash);
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_BASEURL, onepiece->baseUrl);
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_XFER, (void*)onepiece->xfer);

        utstring_free(localfile);
        code = crsync_easy_perform_match(onepiece->crsync_handle);
        if(CRSYNCE_OK == code) {
            elt->size = onepiece->crsync_handle->meta->file_sz;
            elt->diff_size = onepiece->crsync_handle->meta->file_sz - onepiece->crsync_handle->meta->same_count * onepiece->crsync_handle->meta->block_sz;
        } else {
            break;
        }
    }
    utstring_free(hash);
    LOGI("onepiece_perform_MatchRes code = %d\n", code);
    log_dump();
    return code;
}
/*
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
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_HASH, onepiece->magnet->app->hash);
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_BASEURL, onepiece->baseUrl);
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_XFER, (void*)onepiece->xfer);

        code = crsync_easy_perform_match(onepiece->crsync_handle);
        if(CRSYNCE_OK != code) break;
        code = crsync_easy_perform_patch(onepiece->crsync_handle);
        if(CRSYNCE_OK != code) break;
        onepiece->xfer(onepiece->magnet->app->hash, 100);
    } while(0);

    LOGI("onepiece_perform_updateapp code = %d", code);
    return code;
}*/

static curl_off_t localFileResumeSize = 0L;

static int onepiece_updateres_curl_progress(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    const char *hash = (const char*)clientp;
    (void)ultotal;
    (void)ulnow;
    // dltotal dlnow only for current download process, exclude local resume size, so add it here
    int percent = (dltotal > 0) ? ( (dlnow + localFileResumeSize) * 100 / dltotal) : 0;
    double speed = 0;
    CURLcode curlCode = curl_easy_getinfo(onepiece->curl_handle, CURLINFO_SPEED_DOWNLOAD, &speed);
    if(curlCode != CURLE_OK || speed <= 0) {
        speed = 0;
    }
    int isCancel = onepiece->xfer(hash, percent, speed);
    return isCancel;
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
            } else if(CURLE_ABORTED_BY_CALLBACK == curlcode) {
                code = CRSYNCE_CANCEL;
                break;
            }
            else {
                code = CRSYNCE_CURL_ERROR;
            }
        } else {
            code = CRSYNCE_FILE_ERROR;
            break;
        }
        sleep(SLEEP_CURL_RETRY);
        ++retry;
    } while(retry < MAX_CURL_RETRY);
    curl_easy_reset(onepiece->curl_handle);

    utstring_free(url);
    utstring_free(filename);
    LOGI("onepiece_updateres_curl code = %d\n", code);
    return code;
}
/*
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

    res_t *elt=NULL;
    LL_FOREACH(onepiece->magnet->res_list, elt) {
        LOGI("updateres %s %s %d\n", elt->name, elt->hash, elt->size);
        utstring_clear(hash);
        UT_string *localfile = get_full_string( onepiece->localRes, elt->name, NULL);
        //0. if old file not exist, curl download directly, then re-calc hash
        if( -1 == access( utstring_body(localfile), F_OK ) ) {
            //download file
            code = onepiece_updateres_curl(elt->name, elt->hash);
            if(CRSYNCE_OK != code) break;
            //calc hash & string
            rsum_strong_file(utstring_body(localfile), strong);
            for(int j=0; j < STRONG_SUM_SIZE; j++) {
                utstring_printf(hash, "%02x", strong[j] );
            }
            //create localCRC namehash
            UT_string *file_hash = get_full_string(onepiece->localRes, elt->name, utstring_body(hash));
            creat(utstring_body(file_hash), 700);
            utstring_free(file_hash);
        } else {
            UT_string* file_hash = get_full_string(onepiece->localRes, elt->name, elt->hash);
            if(0 == access(utstring_body(file_hash), F_OK)) {
                utstring_printf(hash, "%s", elt->hash);
            } else {
                utstring_printf(hash, "%s", "0000");
                //TODO: maybe should calculate hash again; but this only happen for developer!
            }
            utstring_free(file_hash);
        }
        LOGI("localres hash = %s\n", utstring_body(hash));
        //2. skip update if hash is same
        if(0 == strncmp(utstring_body(hash), elt->hash, strlen(elt->hash))) {
            onepiece->xfer(elt->hash, 100);
            utstring_free(localfile);
            continue;
        }
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_OUTPUTDIR, onepiece->localRes);
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_FILE, utstring_body(localfile));
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_HASH, elt->hash);
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_BASEURL, onepiece->baseUrl);
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_XFER, (void*)onepiece->xfer);

        utstring_free(localfile);
        code = crsync_easy_perform_match(onepiece->crsync_handle);
        if(CRSYNCE_OK != code) break;
        code = crsync_easy_perform_patch(onepiece->crsync_handle);
        if(CRSYNCE_OK != code) break;
        UT_string *oldfile = get_full_string( onepiece->localRes, elt->name, NULL);
        UT_string *newfile = get_full_string( onepiece->localRes, elt->hash, NULL );
        remove( utstring_body(oldfile) );
        code = (0 == rename(utstring_body(newfile), utstring_body(oldfile))) ? CRSYNCE_OK : CRSYNCE_FILE_ERROR;
        utstring_free(oldfile);
        utstring_free(newfile);
        if(CRSYNCE_OK != code) break;
        //create localCRC namehash
        UT_string *file_hash = get_full_string(onepiece->localRes, elt->name, elt->hash);
        creat(utstring_body(file_hash), 700);
        utstring_free(file_hash);
        onepiece->xfer(elt->hash, 100);
        //TODO: Currently lack of remove old file_hash; there will be many file_hash exist at device!
    }
    utstring_free(hash);
    LOGI("onepiece_perform_updateres code = %d\n", code);
    return code;
}*/

CRSYNCcode onepiece_perform_PatchRes() {
    LOGI("onepiece_perform_PatchRes\n");
    CRSYNCcode code = onepiece_checkopt();
    if(CRSYNCE_OK != code) {
        LOGE("onepiece_perform_PatchRes code = %d\n", code);
        return code;
    }

    uint8_t strong[STRONG_SUM_SIZE];
    UT_string *hash = NULL;
    utstring_new(hash);

    res_t *elt=NULL;
    LL_FOREACH(onepiece->magnet->res_list, elt) {
        LOGI("updateres %s %s %d\n", elt->name, elt->hash, elt->size);
        //skip when match result is 100% same
        if(elt->diff_size == 0) {
            onepiece->xfer(elt->hash, 100, 0);
            continue;
        }
        utstring_clear(hash);
        UT_string *localfile = get_full_string( onepiece->localRes, elt->name, NULL);
        //0. if old file not exist, curl download directly, then re-calc hash
        if( -1 == access( utstring_body(localfile), F_OK ) ) {
            //download file
            code = onepiece_updateres_curl(elt->name, elt->hash);
            if(CRSYNCE_OK != code) break;
            //calc hash & string
            rsum_strong_file(utstring_body(localfile), strong);
            for(int j=0; j < STRONG_SUM_SIZE; j++) {
                utstring_printf(hash, "%02x", strong[j] );
            }
            //create localCRC namehash
            UT_string *file_hash = get_full_string(onepiece->localRes, elt->name, utstring_body(hash));
            creat(utstring_body(file_hash), 700);
            utstring_free(file_hash);
        } else {
            UT_string* file_hash = get_full_string(onepiece->localRes, elt->name, elt->hash);
            if(0 == access(utstring_body(file_hash), F_OK)) {
                utstring_printf(hash, "%s", elt->hash);
            } else {
                utstring_printf(hash, "%s", "0000");
                //TODO: maybe should calculate hash again; but this only happen for developer!
            }
            utstring_free(file_hash);
        }
        LOGI("localres hash = %s\n", utstring_body(hash));
        //2. skip update if hash is same
        if(0 == strncmp(utstring_body(hash), elt->hash, strlen(elt->hash))) {
            elt->diff_size = 0;
            onepiece->xfer(elt->hash, 100, 0);
            utstring_free(localfile);
            continue;
        }
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_OUTPUTDIR, onepiece->localRes);
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_FILE, utstring_body(localfile));
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_HASH, elt->hash);
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_BASEURL, onepiece->baseUrl);
        crsync_easy_setopt(onepiece->crsync_handle, CRSYNCOPT_XFER, (void*)onepiece->xfer);

        utstring_free(localfile);
        code = crsync_easy_perform_match(onepiece->crsync_handle);
        if(CRSYNCE_OK != code) break;
        code = crsync_easy_perform_patch(onepiece->crsync_handle);
        if(CRSYNCE_OK != code) break;
        UT_string *oldfile = get_full_string( onepiece->localRes, elt->name, NULL);
        UT_string *newfile = get_full_string( onepiece->localRes, elt->hash, NULL );
        remove( utstring_body(oldfile) );
        code = (0 == rename(utstring_body(newfile), utstring_body(oldfile))) ? CRSYNCE_OK : CRSYNCE_FILE_ERROR;
        utstring_free(oldfile);
        utstring_free(newfile);
        if(CRSYNCE_OK != code) break;
        //create localCRC namehash
        UT_string *file_hash = get_full_string(onepiece->localRes, elt->name, elt->hash);
        creat(utstring_body(file_hash), 700);
        utstring_free(file_hash);
        elt->diff_size = 0;
        onepiece->xfer(elt->hash, 100, 0);
        //TODO: Currently lack of remove old file_hash; there will be many file_hash exist at device!
    }
    utstring_free(hash);
    LOGI("onepiece_perform_PatchRes code = %d\n", code);
    log_dump();
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
    log_dump();
}
