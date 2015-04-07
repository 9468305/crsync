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
#include <time.h>

#include "crsync.h"
#include "curl/curl.h"

#if defined __cplusplus
extern "C" {
#endif

#define LOGI(...)  printf(__VA_ARGS__)

struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = realloc(mem->memory, mem->size + realsize + 1);

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

void testcurl() {
    const char *url = "http://www.qq.com/";
    CURL *curlhandle;
    CURLcode res;
    char error_buf[CURL_ERROR_SIZE];
    long retcode;
    char *contenttype;
    double contentlength;
    double downloadsize;
    struct MemoryStruct chunk;
    chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */
    chunk.size = 0;    /* no data at this point */

    curl_global_init(CURL_GLOBAL_ALL);

    curlhandle = curl_easy_init();

    if (curlhandle) {

        curl_easy_setopt(curlhandle, CURLOPT_PROTOCOLS, CURLPROTO_HTTP); /* http protocol only */
        curl_easy_setopt(curlhandle, CURLOPT_URL, url); /* url */
        curl_easy_setopt(curlhandle, CURLOPT_HTTPGET, 1); /* http get */
        curl_easy_setopt(curlhandle, CURLOPT_FAILONERROR, 1); /* request failure on HTTP response >= 400 */
        curl_easy_setopt(curlhandle, CURLOPT_ERRORBUFFER, error_buf); /* error buffer */
        curl_easy_setopt(curlhandle, CURLOPT_AUTOREFERER, 1); /* allow auto referer */
        curl_easy_setopt(curlhandle, CURLOPT_FOLLOWLOCATION, 1); /* allow follow location */
        curl_easy_setopt(curlhandle, CURLOPT_MAXREDIRS, 5); /* allow redir 5 times */
        curl_easy_setopt(curlhandle, CURLOPT_CONNECTTIMEOUT, 20); /* connection timeout 20s */
        curl_easy_setopt(curlhandle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback); /* receive data callback */
        curl_easy_setopt(curlhandle, CURLOPT_WRITEDATA, (void *)&chunk); /* receive data status */

        res = curl_easy_perform(curlhandle);

        curl_easy_getinfo(curlhandle, CURLINFO_RESPONSE_CODE , &retcode);
        curl_easy_getinfo(curlhandle, CURLINFO_CONTENT_TYPE , &contenttype);
        curl_easy_getinfo(curlhandle, CURLINFO_CONTENT_LENGTH_DOWNLOAD , &contentlength);
        curl_easy_getinfo(curlhandle, CURLINFO_SIZE_DOWNLOAD , &downloadsize);


        LOGI("cur perform result %d\n", res);
        LOGI("curl response code %ld\n", retcode);
        LOGI("curl content type %s\n", contenttype);
        LOGI("curl content length %lf\n", contentlength);
        LOGI("curl download size %lf\n", downloadsize);
        LOGI("%s\n", chunk.memory);

        curl_easy_cleanup(curlhandle);
    }
    free(chunk.memory);
    curl_global_cleanup();
}

int main(void)
{
    testcurl();

    /*
    const char* newFilename = "mthd.apk";
    const char* sigFilename = "mthd.apk.sig";
    const char* oldFilename = "mtsd.apk";
    const char* deltaFilename = "mtsd.apk.delta";

    clock_t t;

    printf("crsync_hatch begin\n");
    t = clock();
    crsync_hatch(newFilename, sigFilename);
    t -= clock();
    printf("crsync_hatch end %ld\n", -t);

    printf("crsync_match begin\n");
    t = clock();
    crsync_match(oldFilename, sigFilename, deltaFilename);
    t -= clock();
    printf("crsync_match end %ld\n", -t);

    printf("crsync_patch begin\n");
    t = clock();
    crsync_patch(oldFilename, deltaFilename, newFilename);
    t -= clock();
    printf("crsync_patch end %ld\n", -t);
*/
    return 0;
}

#if defined __cplusplus
    }
#endif
