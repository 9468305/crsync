rem ndk-build parameters:
rem NDK_PROJECT_PATH =  where is project path
rem NDK_APPLICATION_MK = where is Application.mk
rem APP_BUILD_SCRIPT = where is Android.mk
rem APP_PLATFORM = android-3 default
rem APP_ABI = all or armeabi,armeabi-v7a,x86,mips
rem NDK_DEBUG = 0 or 1
rem NDK_OUT = obj path
rem NDK_LIBS_OUT = libs path
rem clean
rem -B force rebuild

ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=./Android.mk APP_ABI=armeabi-v7a APP_PLATFORM=android-15
