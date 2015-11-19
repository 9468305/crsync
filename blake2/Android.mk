LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := blake2
LOCAL_SRC_FILES := blake2s-ref.c blake2sp-ref.c blake2b-ref.c blake2bp-ref.c md5.c
LOCAL_CFLAGS := -O3 -std=c99 -fopenmp
include $(BUILD_STATIC_LIBRARY)
