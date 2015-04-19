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
#ifndef CRSYNC_LOG_H
#define CRSYNC_LOG_H

#if defined __cplusplus
extern "C" {
#endif

#define CRSYNC_DEBUG 0

#if CRSYNC_DEBUG

#   if defined ANDROID
#       include <android/log.h>
#       define LOG "crsync"
#       define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG, __VA_ARGS__)
#       define LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG, __VA_ARGS__)
#       define LOGW(...)  __android_log_print(ANDROID_LOG_WARN, LOG, __VA_ARGS__)
#       define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG, __VA_ARGS__)
#       define LOGF(...)  __android_log_print(ANDROID_LOG_FATAL, LOG, __VA_ARGS__)
#   else
#       include <stdio.h>
#       define LOGD(...)  printf(__VA_ARGS__)
#       define LOGI(...)  printf(__VA_ARGS__)
#       define LOGW(...)  printf(__VA_ARGS__)
#       define LOGE(...)  printf(__VA_ARGS__)
#       define LOGF(...)  printf(__VA_ARGS__)
#   endif

#else

#   define LOGD(...)
#   define LOGI(...)
#   define LOGW(...)
#   define LOGE(...)
#   define LOGF(...)

#endif

#if defined __cplusplus
    }
#endif

#endif // CRSYNC_LOG_H
