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
    if(!logfile) {
        fopen(LOG_FILENAME, "a");
    }
}

void log_flush() {
    if(logfile) {
        fflush(logfile);
    }
}

void log_close() {
    if(logfile) {
        fclose(logfile);
        logfile = NULL;
    }
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

#if CRSYNC_DEBUG
/*
typedef struct log_t {
    char *str;
    struct log_t *next;
} log_t;

static log_t *log_head = NULL;

static const unsigned int LOG_MAX_SIZE = 256;

void log_append(char *s) {
    log_t *log = malloc(sizeof(log_t));
    log->str = s;
    log->next = NULL;
    LL_APPEND(log_head, log);
    unsigned int count = 0;
    log_t *elt=NULL;
    LL_COUNT(log_head, elt, count);
    if( count >= LOG_MAX_SIZE ) {
        log_dump();
    }
}

void log_dump() {
    FILE *f = fopen(LOG_FILENAME, "a");
    if(f) {
        log_t *elt, *tmp;
        LL_FOREACH_SAFE(log_head,elt,tmp) {
            LL_DELETE(log_head,elt);
            fputs(elt->str, f);
            free(elt->str);
            free(elt);
        }
        log_head = NULL;
        fclose(f);
    }
}

#else

void log_append(char *s){
    free(s);
}

void log_dump(){}

//    char *s=malloc(256);
//    snprintf(s, 256, "%s %d [%s]: " fmt, ts, level, __func__, ##__VA_ARGS__);
//    log_append(s);

*/
#endif //CRSYNC_DEBUG
