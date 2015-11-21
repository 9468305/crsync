@echo off
set ADB=adb.exe
set ARCH=armeabi-v7a
set DEVICE=/data/local/tmp

echo ndk-build blake2test
call %NDKROOT%/ndk-build -B NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=./android-test.mk APP_ABI=armeabi-v7a APP_PLATFORM=android-15 APP_PIE=true

pause
