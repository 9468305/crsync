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
#include <string.h>
#ifdef _MSC_VER
#   include "win/libgen.h"
#else
#   include <libgen.h>
#endif

#include "http.h"
#include "util.h"
#include "log.h"

CRScode HTTP_global_init() {
    CURLcode code = curl_global_init(CURL_GLOBAL_DEFAULT);
    return (code == CURLE_OK) ? CRS_OK : CRS_INIT_ERROR;
}

void HTTP_global_cleanup() {
    curl_global_cleanup();
}

#if 0
static const char s_infotype[CURLINFO_END][3] = {"* ", "< ", "> ", "{ ", "} ", "{ ", "} " };
static int HTTP_curl_debug(CURL *curl, curl_infotype type, char *data, size_t size, void *userptr) {
    (void)curl;
    (void)size;
    (void)userptr;
    switch (type) {
    case CURLINFO_TEXT:
    case CURLINFO_HEADER_IN:
    case CURLINFO_HEADER_OUT:
        LOGD("%s: %s\n", s_infotype[type], data);
        break;
    default:
        break;
    }
    return 0;
}
#endif

static void HTTP_curl_setopt(CURL *curl) {
#if 0
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, HTTP_curl_debug);
#endif
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP); /* http protocol only */
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L); /* request failure on HTTP response >= 400 */
    curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1L); /* allow auto referer */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); /* allow follow location */
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L); /* allow redir 5 times */
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L); /* connection timeout */
    curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 10240L); /* used for check bad network timeout */
    /*Do not setup CURLOPT_TIMEOUT, since range and file download may cost lots of time*/
    /*curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);*/
}

static const char *response200 = "HTTP/1.0 200 OK";
static const char *response200_1 = "HTTP/1.1 200 OK";
static const size_t responselen = 15; //as above strlen

#ifdef _MSC_VER
#   define strcasecmp _stricmp
#   define strncasecmp _strnicmp
#endif

static size_t header_callback(void *data, size_t size, size_t nmemb, void *userp)
{
    (void)userp;
    size_t realSize = size * nmemb;
    size_t minSize = realSize < responselen ? realSize : responselen;
    if( 0 == strncasecmp((const char*)data, response200, minSize) ||
        0 == strncasecmp((const char*)data, response200_1, minSize) ) {
        LOGE("range response 200\n");
        return 0;
    }
    return realSize;
}

CURLcode HTTP_Range(CURL *curl, const char *url, const char *range, void *callback, void *data) {
    HTTP_curl_setopt(curl);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, (void*)header_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl, CURLOPT_RANGE, range);

    CURLcode curlcode = curl_easy_perform(curl);
    if(CURLE_OK != curlcode) {
        LOGI("curlcode %d\n", curlcode);
    }

    curl_easy_reset(curl);
    return curlcode;
}

typedef struct filecache_t {
    const char *name;
    long bytes;
    FILE *file;
} filecache_t;

static size_t HTTP_writefile_func(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    filecache_t * cache = (filecache_t*)userdata;
    size_t w = fwrite(ptr, size, nmemb, cache->file);
    cache->bytes += w;
    int isCancel = 0;
    if(cache->name) {
        isCancel = crs_callback_patch(cache->name, cache->bytes, 0, 0);
    }
    return (isCancel == 0) ? w : 0;
}

CRScode HTTP_File(const char *url, const char *filename, int retry, const char *cbname) {
    if(!url || !filename) {
        LOGE("%d\n", CRS_PARAM_ERROR);
        return CRS_PARAM_ERROR;
    }

    CURL *curl = curl_easy_init();
    if(!curl) {
        LOGE("%d\n", CRS_INIT_ERROR);
        return CRS_INIT_ERROR;
    }

    CRScode code = CRS_OK;
    filecache_t cache;
    cache.name = cbname;

    struct stat st;
    while(retry-- >= 0) {
        if(0 == stat(filename, &st)) {
            cache.bytes = st.st_size;
        } else {
            cache.bytes = 0L;
        }

        FILE *f = fopen(filename, "ab+");
        if(!f) {
            code = CRS_FILE_ERROR;
            break;
        }
        cache.file = f;

        HTTP_curl_setopt(curl);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HTTP_writefile_func);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&cache);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_RESUME_FROM, cache.bytes);

        CURLcode curlcode = curl_easy_perform(curl);
        fclose(f);
        curl_easy_reset(curl);

        switch(curlcode) {
        case CURLE_OK:
            code = CRS_OK;
            break;
        case CURLE_HTTP_RETURNED_ERROR: // server connect OK, remote file not exist
            retry = -1;
            code = CRS_HTTP_ERROR;
            break;
        case CURLE_OPERATION_TIMEDOUT: //timeout, go on
            retry++;
            code = CRS_HTTP_ERROR;
            break;
        default:
            code = CRS_HTTP_ERROR;
            break;
        }

        if(code == CRS_OK) {
            break;
        }
        LOGI("curlcode %d\n", curlcode);

    }//end of while(retry)
    curl_easy_cleanup(curl);

    return code;
}
