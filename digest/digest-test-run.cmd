@echo off
set ADB=adb.exe
set ARCH=armeabi-v7a
set DEVICE=/data/local/tmp
set EXECUTABLE=digest-test

call %ADB% devices

echo deploy file to device
call %ADB% push libs\%ARCH%\%EXECUTABLE% %DEVICE%

echo %EXECUTABLE% start
call %ADB% shell chmod 751 %DEVICE%/%EXECUTABLE%
call %ADB% shell LD_LIBRARY_PATH=%DEVICE% %DEVICE%/%EXECUTABLE%
echo %EXECUTABLE% done

pause
