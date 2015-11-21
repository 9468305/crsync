@echo off
set ADB=adb.exe
set ARCH=armeabi-v7a
set DEVICE=/data/local/tmp

echo check device
call %ADB% devices

echo deploy file to device
call %ADB% push libs\%ARCH%\blake2test %DEVICE%

echo blake2test start
call %ADB% shell chmod 751 %DEVICE%/blake2test
call %ADB% shell LD_LIBRARY_PATH=%DEVICE% %DEVICE%/blake2test
echo blake2test done

pause
