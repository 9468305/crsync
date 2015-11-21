LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := digest-test
LOCAL_SRC_FILES := md5.c blake2s-ref.c blake2sp-ref.c blake2b-ref.c blake2bp-ref.c digest-test.c
LOCAL_CFLAGS := -O3 -std=c99 -fopenmp
LOCAL_LDLIBS := -lc
LOCAL_LDFLAGS := -fopenmp
include $(BUILD_EXECUTABLE)
