#ifndef CRSYNC_LOG_H
#define CRSYNC_LOG_H

#if defined __cplusplus
extern "C" {
#endif

#define CRSYNC_DEBUG 1

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
