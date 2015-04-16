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

/* server side:
 * generate rsums file for filename to outputDir;
 * copy filename to outpuDir;
 * rename filename to hashname at outputDir
*/
void crsync_server(const char *filename, const char *outputDir) {

    //TODO: complete all logic
    crsync_rsums_generate(filename, "mthd.apk.rsums");
}

void client_xfer(int percent) {
    printf("%d", percent);
}
/* client side:
 * update filename to new version;
 * use rsumsURL to run rsync rolling match
 * use newFileURL to download delta data
*/
void crsync_client(const char *filename, const char *rsumsURL, const char *newFileURL) {
    printf("0\n");
    crsync_global_init();
    crsync_handle_t* handle = crsync_easy_init();
    if(handle) {
        crsync_easy_setopt(handle, CRSYNCOPT_ROOT, "./");
        crsync_easy_setopt(handle, CRSYNCOPT_FILE, filename);
        crsync_easy_setopt(handle, CRSYNCOPT_URL, newFileURL);
        crsync_easy_setopt(handle, CRSYNCOPT_SUMURL, rsumsURL);
        crsync_easy_setopt(handle, CRSYNCOPT_XFER, client_xfer);

        crsync_easy_perform(handle);
        printf("1\n");
        crsync_easy_perform(handle);
        printf("2\n");

        crsync_easy_cleanup(handle);
    }
    crsync_global_cleanup();
}


int main(void)
{
    const char *filename = "mtsd.apk";
    const char *rsumsURL = "http://image.ib.qq.com/a/test/mthd.apk.rsums";
    const char *newFileURL = "http://image.ib.qq.com/a/test/mthd.apk";

    printf("crsync main begin\n");
    crsync_client(filename, rsumsURL, newFileURL);

    printf("crsync main end\n");
    return 0;
}

#if defined __cplusplus
    }
#endif
