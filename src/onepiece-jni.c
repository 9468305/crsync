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

#include "log.h"
#include "onepiece.h"

#define ARRAY_COUNT(x) ((int) (sizeof(x) / sizeof((x)[0])))

typedef struct JNIJavaMethods {
    jmethodID   *JavaMethod;
    const char  *FunctionName;
    const char  *FunctionParams;
} JNIJavaMethods;

static jmethodID GMethod_xfer = NULL;

static JavaVM* gJavaVM = NULL;
static jclass gJavaClass = NULL;
static clock_t gClock = 0;

int onepiece_xfer(const char *hash, int percent) {
    clock_t now = clock();
    long diff = (long)(now - gClock);
    if( diff < CLOCKS_PER_SEC && percent != 100) {
        return 0;
    }
    gClock = now;

    JNIEnv *env = NULL;
    int isCancel = 0;
    if ((*gJavaVM)->GetEnv(gJavaVM, (void**)&env, JNI_VERSION_1_6) == JNI_OK) {
        jstring jname = (*env)->NewStringUTF( env, hash );
        isCancel = (*env)->CallStaticIntMethod(env, gJavaClass, GMethod_xfer, jname, (jint)percent);
        (*env)->DeleteLocalRef(env, jname);
    }
    return isCancel;
}

jint JNI_onepiece_init(JNIEnv *env, jclass clazz) {
    return (jint)onepiece_init();
}

jint JNI_onepiece_setopt(JNIEnv *env, jclass clazz, jint opt, jstring j_value) {
    CRSYNCcode code = CRSYNCE_OK;
    const char *value = (*env)->GetStringUTFChars(env, j_value, NULL);
    code = onepiece_setopt(opt,(void*)value);
    (*env)->ReleaseStringUTFChars(env, j_value, value);
    return (jint)code;
}

jstring JNI_onepiece_getinfo_magnet(JNIEnv *env, jclass clazz) {
    UT_string *result = NULL;
    utstring_new(result);

    magnet_t *m = onepiece_getinfo_magnet();
    if(NULL != m) {
        utstring_printf(result, "%s;", m->curr_id);
        utstring_printf(result, "%s;", m->next_id);
        utstring_printf(result, "%s;", m->app->hash);
        utstring_printf(result, "%d;", m->app->size);
        utstring_printf(result, "%d;", m->app->diff_size);
        res_t *elt=NULL;
        LL_FOREACH(m->res_list, elt) {
            utstring_printf(result, "%s;", elt->name);
            utstring_printf(result, "%s;", elt->hash);
            utstring_printf(result, "%d;", elt->size);
            utstring_printf(result, "%d;", elt->diff_size);
        }
    }
    jstring jinfo = (*env)->NewStringUTF( env, utstring_body(result) );
    utstring_free(result);
    return jinfo;
}

jint JNI_onepiece_perform_query(JNIEnv *env, jclass clazz) {
    return (jint)onepiece_perform_query();
}

jint JNI_onepiece_perform_MatchApp(JNIEnv *env, jclass clazz) {
    return (jint)onepiece_perform_MatchApp();
}

jint JNI_onepiece_perform_PatchApp(JNIEnv *env, jclass clazz) {
    gClock = clock();
    onepiece_setopt(ONEPIECEOPT_XFER, (void*)onepiece_xfer);
    return (jint)onepiece_perform_PatchApp();
}

jint JNI_onepiece_perform_MatchRes(JNIEnv *env, jclass clazz) {
    return (jint)onepiece_perform_MatchRes();
}

jint JNI_onepiece_perform_PatchRes(JNIEnv *env, jclass clazz) {
    gClock = clock();
    onepiece_setopt(ONEPIECEOPT_XFER, (void*)onepiece_xfer);
    return (jint)onepiece_perform_PatchRes();
}
/*
jint JNI_onepiece_perform_updateapp(JNIEnv *env, jclass clazz) {
    gClock = clock();
    onepiece_setopt(ONEPIECEOPT_XFER, (void*)onepiece_xfer);
    return (jint)onepiece_perform_updateapp();
}

jint JNI_onepiece_perform_updateres(JNIEnv *env, jclass clazz) {
    gClock = clock();
    onepiece_setopt(ONEPIECEOPT_XFER, (void*)onepiece_xfer);
    return (jint)onepiece_perform_updateres();
}
*/
void JNI_onepiece_cleanup(JNIEnv *env, jclass clazz) {
    onepiece_cleanup();
}

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("onepiece JNI_OnLoad");
    JNIEnv *env;
    if ((*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    // hook up native functions to string names and params
    JNINativeMethod NativeMethods[] = {
        { "JNI_onepiece_init",                 "()I",                      (void*)JNI_onepiece_init               },
        { "JNI_onepiece_setopt",               "(ILjava/lang/String;)I",   (void*)JNI_onepiece_setopt             },
        { "JNI_onepiece_getinfo_magnet",       "()Ljava/lang/String;",     (void*)JNI_onepiece_getinfo_magnet     },
        { "JNI_onepiece_perform_query",        "()I",                      (void*)JNI_onepiece_perform_query      },
        { "JNI_onepiece_perform_MatchApp",     "()I",                      (void*)JNI_onepiece_perform_MatchApp   },
        { "JNI_onepiece_perform_PatchApp",     "()I",                      (void*)JNI_onepiece_perform_PatchApp   },
        { "JNI_onepiece_perform_MatchRes",     "()I",                      (void*)JNI_onepiece_perform_MatchRes   },
        { "JNI_onepiece_perform_PatchRes",     "()I",                      (void*)JNI_onepiece_perform_PatchRes   },
        { "JNI_onepiece_cleanup",              "()V",                      (void*)JNI_onepiece_cleanup            },
    };

    // get the java class from JNI
    jclass crsyncClass = (*env)->FindClass(env, "com/shaddock/crsync/Crsync");
    gJavaClass = (*env)->NewGlobalRef(env, crsyncClass);
    (*env)->RegisterNatives(env, gJavaClass, NativeMethods, ARRAY_COUNT(NativeMethods));

    // hook up java functions to string names and param
    JNIJavaMethods JavaMethods[] = {
        { &GMethod_xfer, "java_onepiece_xfer", "(Ljava/lang/String;I)I" }, //public static int java_onepiece_xfer(String, int);
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
    LOGI("onepiece JNI_OnUnload");
    gJavaVM = NULL;
    JNIEnv *env =NULL;
    if( (*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_6) == JNI_OK ) {
        (*env)->DeleteGlobalRef(env, gJavaClass);
        gJavaClass = NULL;
    }
}
#endif
