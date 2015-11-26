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

#include "define.h"
#include "log.h"
#include "helper.h"
#include "utstring.h"
#include "utlist.h"
#include "http.h"

#define ARRAY_COUNT(x) ((int) (sizeof(x) / sizeof((x)[0])))

typedef struct JNIJavaMethods {
    jmethodID   *JavaMethod;
    const char  *FunctionName;
    const char  *FunctionParams;
} JNIJavaMethods;

static jmethodID gMethod_xfer = NULL;
static JavaVM* gJavaVM = NULL;
static jclass gJavaClass = NULL;
static clock_t gClock = 0;
static bulkHelper_t *gBulkHelper = NULL;

int crsync_progress(const char *basename, const unsigned int bytes, const int isComplete, const int immediate) {
    clock_t now = clock();
    long diff = (long)(now - gClock);
    if( diff < 2*CLOCKS_PER_SEC && immediate == 0 ) {
        return 0;
    }
    gClock = now;

    JNIEnv *env = NULL;
    int isCancel = 0;
    if ((*gJavaVM)->GetEnv(gJavaVM, (void**)&env, JNI_VERSION_1_6) == JNI_OK) {
        jstring jname = (*env)->NewStringUTF( env, basename );
        isCancel = (*env)->CallStaticIntMethod(env, gJavaClass, gMethod_xfer, jname, (jlong)bytes, (jint)isComplete);
        (*env)->DeleteLocalRef(env, jname);
    }
    return isCancel;
}

jint JNI_bulkHelper_init(JNIEnv *env, jclass clazz) {
    CRScode code = HTTP_global_init();
    if(code != CRS_OK) {
        return code;
    }

    free(gBulkHelper);
    gBulkHelper = bulkHelper_malloc();
    return code;
}

jint JNI_bulkHelper_setopt(JNIEnv *env, jclass clazz, jstring j_fileDir, jstring j_baseUrl, jstring j_currVersion) {
    CRScode code = CRS_OK;
    const char *fileDir = (*env)->GetStringUTFChars(env, j_fileDir, NULL);
    const char *baseUrl = (*env)->GetStringUTFChars(env, j_baseUrl, NULL);
    const char *currVersion = (*env)->GetStringUTFChars(env, j_currVersion, NULL);
    gBulkHelper->fileDir = strdup(fileDir);
    gBulkHelper->baseUrl = strdup(baseUrl);
    gBulkHelper->currVersion = strdup(currVersion);
    (*env)->ReleaseStringUTFChars(env, j_fileDir, fileDir);
    (*env)->ReleaseStringUTFChars(env, j_baseUrl, baseUrl);
    (*env)->ReleaseStringUTFChars(env, j_currVersion, currVersion);
    return (jint)code;
}

jint JNI_bulkHelper_perform_magnet(JNIEnv *env, jclass clazz) {
    jint r =(jint)bulkHelper_perform_magnet(gBulkHelper);
    log_dump();
    return r;
}

jstring JNI_bulkHelper_getinfo_magnet(JNIEnv *env, jclass clazz) {
    UT_string *result = NULL;
    utstring_new(result);

    magnet_t *m = gBulkHelper->latestMagnet ? gBulkHelper->latestMagnet : gBulkHelper->magnet;
    if(NULL != m) {
        utstring_printf(result, "%s;", m->currVersion);
        utstring_printf(result, "%s;", m->nextVersion);
        sum_t *elt=NULL;
        LL_FOREACH(m->file, elt) {
            utstring_printf(result, "%s;", elt->name);
            utstring_printf(result, "%d;", elt->size);
        }
    }
    jstring jinfo = (*env)->NewStringUTF( env, utstring_body(result) );
    utstring_free(result);
    return jinfo;
}

jint JNI_bulkHelper_perform_diff(JNIEnv *env, jclass clazz) {
    jint r = (jint)bulkHelper_perform_diff(gBulkHelper);
    log_dump();
    return r;
}

jint JNI_bulkHelper_perform_patch(JNIEnv *env, jclass clazz) {
    jint r = (jint)bulkHelper_perform_patch(gBulkHelper);
    log_dump();
    return r;
}

void JNI_bulkHelper_cleanup(JNIEnv *env, jclass clazz) {
    bulkHelper_free(gBulkHelper);
    gBulkHelper = NULL;
    HTTP_global_cleanup();
    log_dump();
}

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("crsync JNI_OnLoad\n");
    JNIEnv *env;
    if ((*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    // hook up native functions to string names and params
    JNINativeMethod NativeMethods[] = {
        { "JNI_bulkHelper_init",            "()I",                      (void*)JNI_bulkHelper_init               },
        { "JNI_bulkHelper_setopt",          "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I",   (void*)JNI_bulkHelper_setopt },
        { "JNI_bulkHelper_getinfo_magnet",  "()Ljava/lang/String;",     (void*)JNI_bulkHelper_getinfo_magnet     },
        { "JNI_bulkHelper_perform_magnet",  "()I",                      (void*)JNI_bulkHelper_perform_magnet     },
        { "JNI_bulkHelper_perform_diff",    "()I",                      (void*)JNI_bulkHelper_perform_diff      },
        { "JNI_bulkHelper_perform_patch",   "()I",                      (void*)JNI_bulkHelper_perform_patch      },
        { "JNI_bulkHelper_cleanup",         "()V",                      (void*)JNI_bulkHelper_cleanup            },
    };

    // get the java class from JNI
    jclass crsyncClass = (*env)->FindClass(env, "com/shaddock/crsync/Crsync");
    gJavaClass = (*env)->NewGlobalRef(env, crsyncClass);
    (*env)->RegisterNatives(env, gJavaClass, NativeMethods, ARRAY_COUNT(NativeMethods));

    // hook up java functions to string names and param
    JNIJavaMethods JavaMethods[] = {
        { &gMethod_xfer, "java_crsync_progress", "(Ljava/lang/String;JI)I" }, //public static int java_crsync_progress(String, long, int);
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

void JNI_OnUnload(JavaVM* vm, void* InReserved) {
    LOGI("crsync JNI_OnUnload\n");
    gJavaVM = NULL;
    JNIEnv *env =NULL;
    if( (*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_6) == JNI_OK ) {
        (*env)->DeleteGlobalRef(env, gJavaClass);
        gJavaClass = NULL;
    }
}
#endif
