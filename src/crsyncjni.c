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
#include <jni.h>

#include "log.h"
#include "crsync.h"
#include "utstring.h"

#define ARRAY_COUNT(x) ((int) (sizeof(x) / sizeof((x)[0])))

typedef struct JNIJavaMethods {
    jmethodID   *JavaMethod;
    const char  *FunctionName;
    const char  *FunctionParams;
} JNIJavaMethods;

static jmethodID GMethod_callback = NULL;

typedef enum {
    CRSYNCGOPT_MAGNETID = 0,
    CRSYNCGOPT_BASEURL,
    CRSYNCGOPT_LOCALAPP,
    CRSYNCGOPT_LOCALRES,
} CRSYNCGLOBALoption;

typedef enum {
    CRSYNCGINFO_MAGNETID = 0,
} CRSYNCGLOBALinfo;

static crsync_handle_t *gCrsyncHandle = NULL;
static crsync_magnet_t *gMagnet = NULL;
static UT_string *gMagnetID = NULL;
static UT_string *gBaseUrl = NULL;
static UT_string *gLocalApp = NULL;
static UT_string *gLocalRes = NULL;
static int gAction = -1;
static CURL *gCurlHandle = NULL;

static void perform_query_next() {
    
}

static CRSYNCcode perform_updateapp() {
    
}

static CRSYNCcode perform_updateres() {
    CRSYNCcode code;
    
    
    return code;
}

jint native_crsync_init(JNIEnv *env, jclass clazz) {
    LOGI("native_crsync_init");
    CRSYNCcode code = CRSYNCE_OK;
    do {
        if(gCrsyncHandle) {
            code = CRSYNCE_FAILED_INIT;
            break;
        }
        code = crsync_global_init();
        if(CRSYNCE_OK != code) break;
        gCrsyncHandle = crsync_easy_init();
        code = (NULL == gCrsyncHandle) ? CRSYNCE_FAILED_INIT : CRSYNCE_OK;
        if(CRSYNCE_OK != code) break;
        crsync_magnet_new(gMagnet);
        utstring_new(gMagnetID);
        utstring_new(gBaseUrl);
        utstring_new(gLocalApp);
        utstring_new(gLocalRes);
        gCurlHandle = curl_easy_init();
        code = (NULL == gCurlHandle) ? CRSYNCE_FAILED_INIT : CRSYNCE_OK;
    } while(0);
    return (jint)code;
}

jint native_crsync_setopt(JNIEnv *env, jclass clazz, jint opt, jstring j_value) {
    LOGI("native_crsync_setopt %d", opt);
    CRSYNCcode code = CRSYNCE_OK;
    const char *value = (*env)->GetStringUTFChars(env, j_value, NULL);
    do {
        if(!gCrsyncHandle) {
            code = CRSYNCE_INVALID_OPT;
            break;
        }
        switch(opt) {
        case CRSYNCGOPT_MAGNETID:
            utstring_clear(gMagnetID);
            utstring_printf(gMagnetID, "%s", value);
            break;
        case CRSYNCGOPT_BASEURL:
            utstring_clear(gBaseUrl);
            utstring_printf(gBaseUrl, "%s", value);
            break;
        case CRSYNCGOPT_LOCALAPP:
            utstring_clear(gLocalApp);
            utstring_printf(gLocalApp, "%s", value);
            break;
        case CRSYNCGOPT_LOCALRES:
            utstring_clear(gLocalRes);
            utstring_printf(gLocalRes, "%s", value);
            break;
        default:
            LOGE("Unknown opt : %d", opt);
            code = CRSYNCE_INVALID_OPT;
            break;
        }
    } while(0);
    (*env)->ReleaseStringUTFChars(env, j_value, value);
    return (jint)code;
}

jstring native_crsync_getinfo(JNIEnv *env, jclass clazz, jint info) {
    LOGI("native_crsync_getinfo %d", info);
    switch (info) {
    case CRSYNCGINFO_MAGNETID:
        return (*env)->NewStringUTF( env, gMagnet->curr_id );
    default:
        LOGE("Unknown opt : %d", info);
        return (*env)->NewStringUTF( env, "" );
    }
}

jint native_crsync_perform_query(JNIEnv *env, jclass clazz) {
    LOGI("native_crsync_perform_query");
    CRSYNCcode code = CRSYNCE_CURL_ERROR;

    UT_string *magnetUrl = NULL;
    utstring_new(magnetUrl);
    utstring_printf(magnetUrl, "%s%s%s", utstring_body(gBaseUrl), utstring_body(gMagnetID), MAGNET_SUFFIX);

    UT_string *magnetFilename = NULL;
    utstring_new(magnetFilename);
    utstring_printf(magnetFilename, "%s%s%s", utstring_body(gLocalRes), utstring_body(gMagnetID), MAGNET_SUFFIX);

    int retry = MAX_CURL_RETRY;
    do {
        FILE *magnetFile = fopen(utstring_body(magnetFilename), "wb");
        if(magnetFile) {
            crsync_curl_setopt(gCurlHandle);
            curl_easy_setopt(gCurlHandle, CURLOPT_URL, utstring_body(magnetUrl));
            curl_easy_setopt(gCurlHandle, CURLOPT_HTTPGET, 1);
            curl_easy_setopt(gCurlHandle, CURLOPT_WRITEFUNCTION, NULL);
            curl_easy_setopt(gCurlHandle, CURLOPT_WRITEDATA, (void *)magnetFile);
            curl_easy_setopt(gCurlHandle, CURLOPT_NOPROGRESS, 1);
            CURLcode curlcode = curl_easy_perform(gCurlHandle);
            fclose(magnetFile);
            if( CURLE_OK == curlcode) {
                if( CRSYNCE_OK == crsync_sum_check(utstring_body(magnetFilename), MAGNET_TPLMAP_FORMAT) ) {
                    code = CRSYNCE_OK;
                    break;
                }
            }
        } else {
            code = CRSYNCE_FILE_ERROR;
            break;
        }
    } while(retry-- > 0);

    if(CRSYNCE_OK == code) {
        if(gMagnet){
            crsync_magnet_free(gMagnet);
        }
        crsync_magnet_new(gMagnet);
        code = crsync_magnet_load(utstring_body(magnetFilename), gMagnet);
    }

    utstring_free(magnetUrl);
    utstring_free(magnetFilename);

    return (jint)code;
}

jint native_crsync_perform_updateapp(JNIEnv *env, jclass clazz) {
    CRSYNCcode code = CRSYNCE_OK;
    return code;
}

void client_xfer(int percent) {
    LOGI("xfer %d", percent);
}

jint native_crsync_perform_updateres(JNIEnv *env, jclass clazz) {
    LOGI("native_crsync_perform_updateres");
    CRSYNCcode code = CRSYNCE_OK;

    UT_string *url = NULL;
    utstring_new(url);
    
    char **p = NULL, **q = NULL;
    while ( (p=(char**)utarray_next(gMagnet->resname,p)) && (q=(char**)utarray_next(gMagnet->reshash,q)) ) {
        LOGI("updateres %s %s", *p, *q);
        crsync_easy_setopt(gCrsyncHandle, CRSYNCOPT_ROOT, utstring_body(gLocalRes));
        crsync_easy_setopt(gCrsyncHandle, CRSYNCOPT_FILE, *p);

        utstring_clear(url);
        utstring_printf(url, "%s%s", utstring_body(gBaseUrl), *q);
        crsync_easy_setopt(gCrsyncHandle, CRSYNCOPT_URL, utstring_body(url));
        utstring_printf(url, "%s", RSUM_SUFFIX);
        crsync_easy_setopt(gCrsyncHandle, CRSYNCOPT_SUMURL, utstring_body(url));
        crsync_easy_setopt(gCrsyncHandle, CRSYNCOPT_XFER, (void*)client_xfer);

        code = crsync_easy_perform_match(gCrsyncHandle);
        if(CRSYNCE_OK != code) break;
        code = crsync_easy_perform_patch(gCrsyncHandle);
        if(CRSYNCE_OK != code) break;
    }
    utstring_free(url);
    LOGI("native_crsync_perform_updateres code = %d", code);
    return (jint)code;
}

jint native_crsync_cleanup(JNIEnv *env, jclass clazz) {
    LOGI("native_crsync_cleanup");
    CRSYNCcode code = CRSYNCE_OK;
    gAction = -1;
    curl_easy_cleanup(gCurlHandle);
    gCurlHandle = NULL;
    utstring_free(gMagnetID);
    gMagnet = NULL;
    utstring_free(gBaseUrl);
    gBaseUrl = NULL;
    utstring_free(gLocalApp);
    gLocalApp = NULL;
    utstring_free(gLocalRes);
    gLocalRes = NULL;

    crsync_magnet_free(gMagnet);
    crsync_easy_cleanup(gCrsyncHandle);
    gCrsyncHandle = NULL;
    crsync_global_cleanup();
    return (jint)code;
}

void java_crsync_callback() {
    //(*env)->CallStaticVoidMethod(env, clazz, GMethod_callback);
}

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("JNI_OnLoad");
    JNIEnv *env;
    if ((*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    // hook up native functions to string names and params
    JNINativeMethod NativeMethods[] = {
        { "native_crsync_init",                 "()I",                      (void*)native_crsync_init               },
        { "native_crsync_setopt",               "(ILjava/lang/String;)I",   (void*)native_crsync_setopt             },
        { "native_crsync_getinfo",              "(I)Ljava/lang/String;",    (void*)native_crsync_getinfo            },
        { "native_crsync_perform_query",        "()I",                      (void*)native_crsync_perform_query      },
        { "native_crsync_perform_updateapp",    "()I",                      (void*)native_crsync_perform_updateapp  },
        { "native_crsync_perform_updateres",    "()I",                      (void*)native_crsync_perform_updateres  },
        { "native_crsync_cleanup",  "()I",                                  (void*)native_crsync_cleanup            },
    };

    // get the java class from JNI
    jclass crsyncClass = (*env)->FindClass(env, "com/shaddock/crsync/CrsyncJava");
    (*env)->RegisterNatives(env, crsyncClass, NativeMethods, ARRAY_COUNT(NativeMethods));

    // hook up java functions to string names and param
    JNIJavaMethods JavaMethods[] = {
        { &GMethod_callback, "java_callback", "()V" }, //public void java_callback()
    };

    int result = 1;
    for( int MethodIter = 0; MethodIter < ARRAY_COUNT(JavaMethods); MethodIter++ ) {
        *JavaMethods[MethodIter].JavaMethod = (*env)->GetStaticMethodID(env, crsyncClass, JavaMethods[MethodIter].FunctionName, JavaMethods[MethodIter].FunctionParams);
        if( 0 == *JavaMethods[MethodIter].JavaMethod ) {
            LOGE("JavaMethod not found! %s(%s)", JavaMethods[MethodIter].FunctionName, JavaMethods[MethodIter].FunctionParams);
            result = 0;
            break;
        }
    }
    // clean up references
    (*env)->DeleteLocalRef(env, crsyncClass);
    return (result == 1) ? JNI_VERSION_1_6 : JNI_ERR;
}

void JNI_OnUnload(JavaVM* vm, void* InReserved) {
    LOGI("JNI_OnUnload called");
}
