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
#ifdef ANDROID
#include <jni.h>
#include <time.h>

#include "global.h"
#include "log.h"
#include "helper.h"
#include "utstring.h"
#include "utlist.h"
#include "util.h"
#include "http.h"

#define ARRAY_COUNT(x) ((int) (sizeof(x) / sizeof((x)[0])))

typedef struct JNIJavaMethods {
    jmethodID   *JavaMethod;
    const char  *FunctionName;
    const char  *FunctionParams;
} JNIJavaMethods;

static JavaVM* gJavaVM = NULL;
static jclass gJavaClass = NULL;
static jmethodID gMethod_onProgress = NULL;
static jmethodID gMethod_onDiff = NULL;

static clock_t gClock = 0;

static bulkHelper_t *gBulkHelper = NULL;

int crsync_progress(const char *basename, const unsigned int bytes, const int isComplete, const int immediate) {
    clock_t now = clock();
    long diff = (long)(now - gClock);
    if( diff < CLOCKS_PER_SEC && immediate == 0 ) {
        return 0;
    }
    gClock = now;

    JNIEnv *env = NULL;
    int isCancel = 0;
    if ((*gJavaVM)->GetEnv(gJavaVM, (void**)&env, JNI_VERSION_1_6) == JNI_OK) {
        jstring jname = (*env)->NewStringUTF( env, basename );
        isCancel = (*env)->CallStaticIntMethod(env, gJavaClass, gMethod_onProgress, jname, (jlong)bytes, (jint)isComplete);
        (*env)->DeleteLocalRef(env, jname);
    }
    return isCancel;
}

void crsync_diff(const char *basename, const unsigned int bytes, const int isComplete, const int isNew) {
    JNIEnv *env = NULL;
    if ((*gJavaVM)->GetEnv(gJavaVM, (void**)&env, JNI_VERSION_1_6) == JNI_OK) {
        jstring jname = (*env)->NewStringUTF( env, basename );
        (*env)->CallStaticVoidMethod(env, gJavaClass, gMethod_onDiff, jname, (jlong)bytes, (jint)isComplete, (jint)isNew);
        (*env)->DeleteLocalRef(env, jname);
    }
}

jint JNI_crsync_init(JNIEnv *env, jclass clazz, jstring j_fileDir, jstring j_baseUrl, jstring j_currVersion) {
    const char *fileDir = (*env)->GetStringUTFChars(env, j_fileDir, NULL);
    const char *baseUrl = (*env)->GetStringUTFChars(env, j_baseUrl, NULL);
    const char *currVersion = (*env)->GetStringUTFChars(env, j_currVersion, NULL);

    CRScode code = CRS_OK;
    do {
        log_open();
        code = HTTP_global_init();
        if(code != CRS_OK) break;
        free(gBulkHelper);
        gBulkHelper = bulkHelper_malloc();
        gBulkHelper->fileDir = strdup(fileDir);
        gBulkHelper->baseUrl = strdup(baseUrl);
        gBulkHelper->currentVersion = strdup(currVersion);
    } while(0);

    (*env)->ReleaseStringUTFChars(env, j_fileDir, fileDir);
    (*env)->ReleaseStringUTFChars(env, j_baseUrl, baseUrl);
    (*env)->ReleaseStringUTFChars(env, j_currVersion, currVersion);
    return (jint)code;
}

jint JNI_crsync_perform_magnet(JNIEnv *env, jclass clazz) {
    jint r = (jint)bulkHelper_perform_magnet(gBulkHelper);
    return r;
}

jstring JNI_crsync_get_magnet(JNIEnv *env, jclass clazz, jint isLatest) {
    UT_string *result = NULL;
    utstring_new(result);

    magnet_t *m = (isLatest == 0) ? gBulkHelper->currentMagnet : gBulkHelper->latestMagnet;
    if(m) {
        utstring_printf(result, "%s;", m->currVersion);
        sum_t *elt=NULL;
        LL_FOREACH(m->file, elt) {
            utstring_printf(result, "%s;", elt->name);
            char * hashStr = Util_hex_string(elt->digest, CRS_STRONG_DIGEST_SIZE);
            utstring_printf(result, "%s;", hashStr);
            free(hashStr);
            utstring_printf(result, "%u;", elt->size);
        }
    }
    jstring jinfo = (*env)->NewStringUTF( env, utstring_body(result) );
    utstring_free(result);
    return jinfo;
}

jint JNI_crsync_perform_diff(JNIEnv *env, jclass clazz) {
    jint r = (jint)bulkHelper_perform_diff(gBulkHelper);
    return r;
}

jint JNI_crsync_perform_patch(JNIEnv *env, jclass clazz, jint isLatest) {
    jint r = (jint)bulkHelper_perform_patch(gBulkHelper, (int)isLatest);
    return r;
}

void JNI_crsync_cleanup(JNIEnv *env, jclass clazz) {
    bulkHelper_free(gBulkHelper);
    gBulkHelper = NULL;
    HTTP_global_cleanup();
    log_close();
}

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    log_open();
    LOGI("crsync JNI_OnLoad\n");
    JNIEnv *env;
    if ((*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    // hook up native functions to string names and params
    JNINativeMethod NativeMethods[] = {
        { "JNI_crsync_init",            "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I", (void*)JNI_crsync_init },
        { "JNI_crsync_get_magnet",      "(I)Ljava/lang/String;",    (void*)JNI_crsync_get_magnet },
        { "JNI_crsync_perform_magnet",  "()I",                      (void*)JNI_crsync_perform_magnet },
        { "JNI_crsync_perform_diff",    "()I",                      (void*)JNI_crsync_perform_diff },
        { "JNI_crsync_perform_patch",   "(I)I",                     (void*)JNI_crsync_perform_patch },
        { "JNI_crsync_cleanup",         "()V",                      (void*)JNI_crsync_cleanup },
    };

    // get the java class from JNI
    jclass crsyncClass = (*env)->FindClass(env, "com/shaddock/crsync/Crsync");
    gJavaClass = (*env)->NewGlobalRef(env, crsyncClass);
    (*env)->RegisterNatives(env, gJavaClass, NativeMethods, ARRAY_COUNT(NativeMethods));

    // hook up java functions to string names and param
    JNIJavaMethods JavaMethods[] = {
        { &gMethod_onProgress,  "java_onProgress",  "(Ljava/lang/String;JI)I" }, //public static int java_onProgress(String, long, int);
        { &gMethod_onDiff,      "java_onDiff",      "(Ljava/lang/String;JII)V" },//public static void java_onDiff(String, long, int, int);
    };

    int result = 1;
    for( int MethodIter = 0; MethodIter < ARRAY_COUNT(JavaMethods); MethodIter++ ) {
        *JavaMethods[MethodIter].JavaMethod = (*env)->GetStaticMethodID(env, gJavaClass, JavaMethods[MethodIter].FunctionName, JavaMethods[MethodIter].FunctionParams);
        if( 0 == *JavaMethods[MethodIter].JavaMethod ) {
            LOGE("JavaMethod not found! %s(%s)", JavaMethods[MethodIter].FunctionName, JavaMethods[MethodIter].FunctionParams);
            result = 0;
            break;
        }
    }
    gJavaVM = vm;
    return (result == 1) ? JNI_VERSION_1_6 : JNI_ERR;
}

/* At Android Platform, JNI_OnUnload() will never been called */
void JNI_OnUnload(JavaVM* vm, void* InReserved) {
    log_close();
    gJavaVM = NULL;
    JNIEnv *env =NULL;
    if( (*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_6) == JNI_OK ) {
        (*env)->DeleteGlobalRef(env, gJavaClass);
        gJavaClass = NULL;
    }
}

#endif
