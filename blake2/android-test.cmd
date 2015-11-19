@echo off
set ADB=adb.exe
set ARCH=armeabi-v7a
set DEVICE=/data/local/tmp

echo ndk-build blake2test
call %NDKROOT%/ndk-build -B NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=./android-test.mk APP_ABI=armeabi-v7a APP_PLATFORM=android-15 APP_PIE=true

echo check device
call %ADB% devices

echo deploy file to device
call %ADB% push libs\%ARCH%\blake2test %DEVICE%

echo blake2test start
call %ADB% shell chmod 751 %DEVICE%/blake2test
call %ADB% shell LD_LIBRARY_PATH=%DEVICE% %DEVICE%/blake2test
echo blake2test done

pause
