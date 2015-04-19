#include <jni.h>

#include "log.h"

# define ARRAY_COUNT(x) ((int) (sizeof(x) / sizeof((x)[0])))

typedef struct JNIJavaMethods
{
    jmethodID   *JavaMethod;
    const char  *FunctionName;
    const char  *FunctionParams;
} JNIJavaMethods;

static jmethodID GMethod_callback;

void native_crsync_main(JNIEnv *env, jclass clazz) {
    (*env)->CallStaticVoidMethod(env, clazz, GMethod_callback);
}

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv *env;
    (void*)reserved;

    if ((*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_6) != JNI_OK)
    {
        return JNI_ERR;
    }

    // hook up native functions to string names and params
    JNINativeMethod NativeMethods[] =
    {
        { "native_crsync_main", "()V", (void*)native_crsync_main },
    };

    // get the java class from JNI
    jclass crsyncClass;
    crsyncClass = (*env)->FindClass(env, "com/shaddock/crsync/CrsyncJava");
    (*env)->RegisterNatives(env, crsyncClass, NativeMethods, ARRAY_COUNT(NativeMethods));

    // hook up java functions to string names and param
    JNIJavaMethods JavaMethods[] =
    {
        //public void java_callback()
        { &GMethod_callback, "java_callback", "()V" },
    };

    int result = 1;
    for( int MethodIter = 0; MethodIter < ARRAY_COUNT(JavaMethods); MethodIter++ )
    {
        *JavaMethods[MethodIter].JavaMethod = (*env)->GetStaticMethodID(env, crsyncClass, JavaMethods[MethodIter].FunctionName, JavaMethods[MethodIter].FunctionParams);
        if( 0 == *JavaMethods[MethodIter].JavaMethod )
        {
            LOGE("JavaMethod not found! %s(%s)", JavaMethods[MethodIter].FunctionName, JavaMethods[MethodIter].FunctionParams);
            result = 0;
            break;
        }
    }
    // clean up references
    (*env)->DeleteLocalRef(env, crsyncClass);

    return (result == 1) ? JNI_VERSION_1_6 : JNI_ERR;
}

void JNI_OnUnload(JavaVM* vm, void* InReserved)
{
    LOGI("JNI_OnUnload called");
}
