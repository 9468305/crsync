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

#include "http.h"
#include "util.h"
#include "curl/curl.h"
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
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 20L); /* connection timeout 20s */
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
}

static const char *response200 = "HTTP/1.0 200 OK";
static const char *response200_1 = "HTTP/1.1 200 OK";
static const size_t responselen = 15; //as above strlen

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

CRScode HTTP_Range(const char *url, const char *range, void *callback, void *data) {
    if(!url || !range || !callback) {
        LOGE("%d\n", CRS_PARAM_ERROR);
        return CRS_PARAM_ERROR;
    }

    CURL *curl = curl_easy_init();
    if(!curl) {
        LOGE("%d\n", CRS_HTTP_ERROR);
        return CRS_HTTP_ERROR;
    }

    CRScode code = CRS_OK;

    HTTP_curl_setopt(curl);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, (void*)header_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl, CURLOPT_RANGE, range);

    CURLcode curlcode = curl_easy_perform(curl);
    curl_easy_reset(curl);
    switch(curlcode) {
    case CURLE_OK:
        code = CRS_OK;
        break;
    default:
        code = CRS_HTTP_ERROR;
        LOGE("curlcode %d\n", curlcode);
        break;
    }

    curl_easy_cleanup(curl);
    return code;
}
/*
typedef struct datacache_t {
    CURL *curl;
    unsigned char *buf;
    size_t len;
    size_t pos;
} datacache_t;

CRScode HTTP_Data(const char *url, const char *range, unsigned char *out, unsigned int outlen, int retry) {
    if(!url || !out) {
        LOGE("%d\n", CRS_PARAM_ERROR);
        return CRS_PARAM_ERROR;
    }

    CURL *curl = curl_easy_init();
    if(!curl) {
        LOGE("%d\n", CRS_HTTP_ERROR);
        return CRS_HTTP_ERROR;
    }

    CRScode code = CRS_OK;
    datacache_t cache;
    while(retry-- >= 0) {
        cache.curl = curl;
        cache.buf = out;
        cache.len = outlen;
        cache.pos = 0;

        HTTP_curl_setopt(curl);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HTTP_writedata_func);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&cache);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_RANGE, range);

        CURLcode curlcode = curl_easy_perform(curl);
        curl_easy_reset(curl);
        switch(curlcode) {
        case CURLE_OK:
            code = CRS_OK;
            break;
        case CURLE_WRITE_ERROR:
            code = CRS_HTTP_ERROR; //write buffer error since HTTP-Response 200
            break;
        default:
            code = CRS_HTTP_ERROR;
            break;
        }
        if(code == CRS_OK) {
            break;
        } else {
            LOGI("curlcode %d\n", curlcode);
        }
    }//end of while(retry)

    curl_easy_cleanup(curl);
    return code;
}
*/

typedef struct filecache_t {
    const char *url;
    long bytes;
    FILE *file;
    HTTP_callback *cb;
    int cancel;
} filecache_t;

static size_t HTTP_writefile_func(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    filecache_t * cache = (filecache_t*)userdata;
    size_t w = fwrite(ptr, size, nmemb, cache->file);
    if(cache->cb) {
        cache->cancel = cache->cb(cache->url, cache->bytes);
    }
    return cache->cancel ? 0 : w;
}

CRScode HTTP_File(const char *url, const char *filename, int retry, HTTP_callback *cb) {
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

    while(retry-- >= 0) {
        cache.url = url;
        cache.cb = cb;
        cache.cancel = 0;

        struct stat st;
        if(!stat(filename, &st)) {
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
        curl_easy_reset(curl);
        fclose(f);
        switch(curlcode) {
        case CURLE_OK:
            code = CRS_OK;
            break;
        case CURLE_WRITE_ERROR:
            code = cache.cancel ? CRS_USER_CANCEL : CRS_FILE_ERROR;
            break;
        default:
            code = CRS_HTTP_ERROR;
            LOGI("curlcode %d\n", curlcode);
            break;
        }

        if(code == CRS_OK) {
            break;
        }
        if(code == CRS_USER_CANCEL) {
            LOGI("user cancel\n");
            break;
        }
    }//end of while(retry)
    curl_easy_cleanup(curl);

    return code;
}
