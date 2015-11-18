@echo off
call %NDKROOT%/ndk-build -B NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=./Android.mk APP_ABI=armeabi-v7a APP_PLATFORM=android-15
pause
