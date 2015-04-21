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
    CRSYNCGA_Query = 0,
    CRSYNCGA_UpdateApp,
    CRSYNCGA_UpdateRes,
} CRSYNCGaction;

typedef enum {
    CRSYNCGOPT_MAGNETID = 0,
    CRSYNCGOPT_BASEURL,
    CRSYNCGOPT_LOCALAPP,
    CRSYNCGOPT_LOCALRES,
    CRSYNCGOPT_ACTION,

} CRSYNCBLOBALoption;

static crsync_handle_t *gHandle = NULL;
static crsync_magnet_t *gMagnet = NULL;
static UT_string *gMagnetID = NULL;
static UT_string *gBaseUrl = NULL;
static UT_string *gLocalApp = NULL;
static UT_string *gLocalRes = NULL;
static int gAction = -1;
static CURL *gCurlHandle = NULL;

static void perform_query_next() {
    
}

static CRSYNCcode perform_query() {
    CRSYNCcode code;

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
        crsync_magnet_free(gMagnet);
        code = crsync_magnet_load(utstring_body(magnetFilename), gMagnet);
    }

    utstring_free(magnetUrl);
    utstring_free(magnetFilename);

    return code;
}

static CRSYNCcode perform_updateapp() {
    CRSYNCcode code;
    
    
    return code;
}

static CRSYNCcode perform_updateres() {
    CRSYNCcode code;
    
    
    return code;
}

jint native_crsync_init(JNIEnv *env, jclass clazz) {
    LOGI("native_crsync_init");
    CRSYNCcode code = CRSYNCE_OK;
    do {
        if(gHandle) {
            code = CRSYNCE_FAILED_INIT;
            break;
        }
        code = crsync_global_init();
        if(CRSYNCE_OK != code) break;
        gHandle = crsync_easy_init();
        code = (NULL == gHandle) ? CRSYNCE_FAILED_INIT : CRSYNCE_OK;
        if(CRSYNCE_OK != code) break;
        gMagnet = calloc(1, sizeof(crsync_magnet_t));
        utstring_new(gMagnetID);
        utstring_new(gBaseUrl);
        utstring_new(gLocalApp);
        utstring_new(gLocalRes);
        gCurlHandle = curl_easy_init();
        code = (NULL == gCurlHandle) ? CRSYNCE_FAILED_INIT : CRSYNCE_OK;
    } while(0);
    return (jint)code;
}

jint native_crsync_setopt(JNIEnv *env, jclass clazz, int opt, jstring j_value) {
    LOGI("native_crsync_setopt");
    CRSYNCcode code = CRSYNCE_OK;
    const char *value = (*env)->GetStringUTFChars(env, j_value, NULL);
    do {
        if(!gHandle) {
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
        case CRSYNCGOPT_ACTION:
            gAction = atoi(value);
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

jstring native_crsync_getinfo(JNIEnv *env, jclass clazz, int info) {
    LOGI("native_crsync_getinfo");
    CRSYNCcode code = CRSYNCE_OK;
    jstring result;
    return result;
}

jint native_crsync_perform(JNIEnv *env, jclass clazz) {
    LOGI("native_crsync_perform");
    CRSYNCcode code = CRSYNCE_OK;
    switch(gAction) {
    case CRSYNCGA_Query:
        code = perform_query();
        break;
    case CRSYNCGA_UpdateApp:
        code = perform_updateapp();
        break;
    case CRSYNCGA_UpdateRes:
        code = perform_updateres();
        break;
    default:
        LOGE("Unknown action : %d", gAction);
        code = CRSYNCE_INVALID_OPT;
        break;
    }
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
    gMagnet = NULL;
    crsync_easy_cleanup(gHandle);
    gHandle = NULL;
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
        { "native_crsync_init",     "()I",                      (void*)native_crsync_init       },
        { "native_crsync_setopt",   "(ILjava/lang/String;)I",   (void*)native_crsync_setopt     },
        { "native_crsync_getinfo",  "(I)Ljava/lang/String;",    (void*)native_crsync_getinfo    },
        { "native_crsync_perform",  "()I",                      (void*)native_crsync_perform    },
        { "native_crsync_cleanup",  "()I",                      (void*)native_crsync_cleanup    },
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
