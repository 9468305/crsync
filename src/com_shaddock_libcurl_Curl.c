#include "com_shaddock_libcurl_Curl.h"

#include "crsync.h"
#include "log.h"

void client_xfer(int percent) {
    LOGI("%d", percent);
}

JNIEXPORT void JNICALL Java_com_shaddock_libcurl_Curl_maintest(JNIEnv *env, jclass cls) {
    crsync_global_init();
    crsync_handle_t* handle = crsync_easy_init();
    if(handle) {
        crsync_easy_setopt(handle, CRSYNCOPT_ROOT, "/sdcard/curl/");
        crsync_easy_setopt(handle, CRSYNCOPT_FILE, "mtsd.apk");
        crsync_easy_setopt(handle, CRSYNCOPT_URL, "http://image.ib.qq.com/a/test/mthd.apk");
        crsync_easy_setopt(handle, CRSYNCOPT_SUMURL, "http://image.ib.qq.com/a/test/mthd.apk.rsums");
        crsync_easy_setopt(handle, CRSYNCOPT_XFER, client_xfer);

        LOGI("0\n");
        crsync_easy_perform(handle);
        LOGI("1\n");
        crsync_easy_perform(handle);
        LOGI("2\n");

        crsync_easy_cleanup(handle);
    }
    crsync_global_cleanup();
}
