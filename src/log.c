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
#include <time.h>

#include "log.h"
#include "utlist.h"

#if defined ANDROID
#   define LOG_FILENAME "/sdcard/crsync_core.log"
#else
#   define LOG_FILENAME "crsync_core.log"
#endif

FILE *logfile = NULL;

void log_open() {
#if CRSYNC_DEBUG
    if(!logfile) logfile = fopen(LOG_FILENAME, "at+");
#endif
}

void log_close() {
#if CRSYNC_DEBUG
    if(logfile) {
        fflush(logfile);
        fclose(logfile);
        logfile = NULL;
    }
#endif
}

void log_timestamp(char *ts) {
    time_t t;
    time(&t);
#if defined ANDROID
    struct tm tmv;
    localtime_r(&t, &tmv);
    strftime(ts, LOG_TIME_STRING_SIZE, "%Y%m%d %H:%M:%S", &tmv);
#else
    struct tm *tmv = localtime(&t);
    strftime(ts, LOG_TIME_STRING_SIZE, "%Y%m%d %H:%M:%S", tmv);
#endif
}
