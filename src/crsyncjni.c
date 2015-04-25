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

typedef enum {
    ONEPIECEOPT_MAGNETID = 0,
    ONEPIECEOPT_BASEURL,
    ONEPIECEOPT_LOCALAPP,
    ONEPIECEOPT_LOCALRES,
} ONEPIECEoption;

typedef enum {
    ONEPIECEINFO_QUERY = 0,
} ONEPIECEinfo;

static JavaVM* gJavaVM = NULL;
static jclass gJavaClass = NULL;
static crsync_handle_t *gCrsyncHandle = NULL;
static magnet_t *gMagnet = NULL;
static char *gMagnetID = NULL;
static char *gBaseUrl = NULL;
static char *gLocalApp = NULL;
static char *gLocalRes = NULL;
static CURL *gCurlHandle = NULL;
static clock_t gClock = 0;

static void perform_query_next() {
    
}

static CRSYNCcode perform_updateapp() {
    
}

static CRSYNCcode perform_updateres() {
    CRSYNCcode code;
    
    
    return code;
}

void onepiece_xfer(const char *name, int percent) {
    clock_t now = clock();
    long diff = (long)(now - gClock);
    if( diff < CLOCKS_PER_SEC && percent != 100) {
        return;
    }
    gClock = now;

    JNIEnv *env = NULL;
    if ((*gJavaVM)->GetEnv(gJavaVM, (void**)&env, JNI_VERSION_1_6) == JNI_OK) {
        jstring jname = (*env)->NewStringUTF( env, name );
        (*env)->CallStaticVoidMethod(env, gJavaClass, GMethod_xfer, jname, (jint)percent);
        (*env)->DeleteLocalRef(env, jname);
    }
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
        onepiece_magnet_new(gMagnet);
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
        case ONEPIECEOPT_MAGNETID:
            if(gMagnetID) {
                free(gMagnetID);
            }
            gMagnetID = strdup( value );
            break;
        case ONEPIECEOPT_BASEURL:
            if(gBaseUrl) {
                free(gBaseUrl);
            }
            gBaseUrl = strdup( value );
            break;
        case ONEPIECEOPT_LOCALAPP:
            if(gLocalApp) {
                free(gLocalApp);
            }
            gLocalApp = strdup( value );
            break;
        case ONEPIECEOPT_LOCALRES:
            if(gLocalRes) {
                free(gLocalRes);
            }
            gLocalRes = strdup( value );
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
    case ONEPIECEINFO_QUERY:
    {
        UT_string *info = NULL;
        utstring_new(info);
        if(gMagnet) {
            utstring_printf(info, "%s;", gMagnet->curr_id);
            utstring_printf(info, "%s;", gMagnet->next_id);
            utstring_printf(info, "%s;", gMagnet->appname);
            utstring_printf(info, "%s;", gMagnet->apphash);
            char **p = NULL, **q = NULL;
            while ( (p=(char**)utarray_next(gMagnet->resname,p)) && (q=(char**)utarray_next(gMagnet->reshash,q)) ) {
                utstring_printf(info, "%s;", *p);
                utstring_printf(info, "%s;", *q);
            }
        }
        jstring jinfo = (*env)->NewStringUTF( env, utstring_body(info) );
        utstring_free(info);
        return jinfo;
    }
    default:
        LOGE("Unknown opt : %d", info);
        return (*env)->NewStringUTF( env, "" );
    }
}

jint native_crsync_perform_query(JNIEnv *env, jclass clazz) {
    LOGI("native_crsync_perform_query");
    CRSYNCcode code = CRSYNCE_CURL_ERROR;

    UT_string *magnetUrl = get_full_string(gBaseUrl, gMagnetID, MAGNET_SUFFIX);
    UT_string *magnetFilename = get_full_string(gLocalRes, gMagnetID, MAGNET_SUFFIX);

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
                if( CRSYNCE_OK == crsync_tplfile_check(utstring_body(magnetFilename), MAGNET_TPLMAP_FORMAT) ) {
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
            onepiece_magnet_free(gMagnet);
        }
        onepiece_magnet_new(gMagnet);
        code = onepiece_magnet_load(utstring_body(magnetFilename), gMagnet);
    }

    utstring_free(magnetUrl);
    utstring_free(magnetFilename);

    return (jint)code;
}

jint native_crsync_perform_updateapp(JNIEnv *env, jclass clazz) {
    CRSYNCcode code = CRSYNCE_OK;
    return code;
}

jint native_crsync_perform_updateres(JNIEnv *env, jclass clazz) {
    LOGI("native_crsync_perform_updateres");
    CRSYNCcode code = CRSYNCE_OK;
    uint8_t strong[STRONG_SUM_SIZE];
    UT_string *hash = NULL;
    utstring_new(hash);
    char **p = NULL, **q = NULL;
    gClock = clock();
    while ( (p=(char**)utarray_next(gMagnet->resname,p)) && (q=(char**)utarray_next(gMagnet->reshash,q)) ) {
        LOGI("updateres %s %s", *p, *q);
        //first check old file's strongsum hash with new file's
        UT_string *file = get_full_string( gLocalRes, *p, NULL);
        rsum_strong_file(utstring_body(file), strong);
        utstring_free(file);
        utstring_clear(hash);
        for(int j=0; j < STRONG_SUM_SIZE; j++) {
            utstring_printf(hash, "%02x", strong[j] );
        }
        if(0 == strncmp(utstring_body(hash), *q, STRONG_SUM_SIZE*2)) {
            onepiece_xfer(*p, 100);
            continue;
        }
        crsync_easy_setopt(gCrsyncHandle, CRSYNCOPT_OUTPUTDIR, gLocalRes);
        crsync_easy_setopt(gCrsyncHandle, CRSYNCOPT_FILE, *p);
        crsync_easy_setopt(gCrsyncHandle, CRSYNCOPT_HASH, *q);
        crsync_easy_setopt(gCrsyncHandle, CRSYNCOPT_BASEURL, gBaseUrl);
        crsync_easy_setopt(gCrsyncHandle, CRSYNCOPT_XFER, (void*)onepiece_xfer);

        code = crsync_easy_perform_match(gCrsyncHandle);
        if(CRSYNCE_OK != code) break;
        code = crsync_easy_perform_patch(gCrsyncHandle);
        if(CRSYNCE_OK != code) break;
        UT_string *old = get_full_string( gLocalRes, *p, NULL);
        UT_string *new = get_full_string( gLocalRes, *q, NULL );
        remove( utstring_body(old) );
        code = (0 == rename(utstring_body(new), utstring_body(old))) ? CRSYNCE_OK : CRSYNCE_FILE_ERROR;
        utstring_free(old);
        utstring_free(new);
        if(CRSYNCE_OK != code) break;
        onepiece_xfer(*p, 100);
    }

    utstring_free(hash);
    LOGI("native_crsync_perform_updateres code = %d", code);
    return (jint)code;
}

jint native_crsync_cleanup(JNIEnv *env, jclass clazz) {
    LOGI("native_crsync_cleanup");
    CRSYNCcode code = CRSYNCE_OK;
    curl_easy_cleanup(gCurlHandle);
    gCurlHandle = NULL;
    free(gMagnetID);
    gMagnetID = NULL;
    free(gBaseUrl);
    gBaseUrl = NULL;
    free(gLocalApp);
    gLocalApp = NULL;
    free(gLocalRes);
    gLocalRes = NULL;

    onepiece_magnet_free(gMagnet);
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
    gJavaClass = (*env)->FindClass(env, "com/shaddock/crsync/CrsyncJava");
    (*env)->NewGlobalRef(env, gJavaClass);
    (*env)->RegisterNatives(env, gJavaClass, NativeMethods, ARRAY_COUNT(NativeMethods));

    // hook up java functions to string names and param
    JNIJavaMethods JavaMethods[] = {
        { &GMethod_xfer, "java_onepiece_xfer", "(Ljava/lang/String;I)V" }, //public static void java_onepiece_xfer(String, int);
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
    LOGI("JNI_OnUnload called");
    gJavaVM = NULL;
    JNIEnv *env =NULL;
    if( (*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_6) == JNI_OK ) {
        (*env)->DeleteGlobalRef(env, gJavaClass);
        gJavaClass = NULL;
    }
}
