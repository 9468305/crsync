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

#define CRSYNC_DEBUG 1

#if CRSYNC_DEBUG

#   if defined ANDROID
#       include <android/log.h>
#       define LOG_TAG "crsync_ndk"
#       define LOGD(fmt, ...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "[%s]: " fmt, __func__, ##__VA_ARGS__)
#       define LOGI(fmt, ...)  __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, "[%s]: " fmt, __func__, ##__VA_ARGS__)
#       define LOGW(fmt, ...)  __android_log_print(ANDROID_LOG_WARN,  LOG_TAG, "[%s]: " fmt, __func__, ##__VA_ARGS__)
#       define LOGE(fmt, ...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "[%s]: " fmt, __func__, ##__VA_ARGS__)
#   else
#       include <stdio.h>
#       define LOGD(fmt, ...)  printf("D [%s]: " fmt, __func__, ##__VA_ARGS__)
#       define LOGI(fmt, ...)  printf("I [%s]: " fmt, __func__, ##__VA_ARGS__)
#       define LOGW(fmt, ...)  printf("W [%s]: " fmt, __func__, ##__VA_ARGS__)
#       define LOGE(fmt, ...)  printf("E [%s]: " fmt, __func__, ##__VA_ARGS__)
#   endif

#else

#   define LOGD(...)
#   define LOGI(...)
#   define LOGW(...)
#   define LOGE(...)

#endif

#if defined __cplusplus
    }
#endif

#endif // CRSYNC_LOG_H
