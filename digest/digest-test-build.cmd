@echo off
call %NDKROOT%/ndk-build -B NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=./digest-test.mk APP_ABI=armeabi-v7a APP_PLATFORM=android-15 APP_PIE=true
pause
