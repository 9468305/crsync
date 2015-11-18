LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := curl
LOCAL_SRC_FILES := $(LOCAL_PATH)/../libcurl/lib/$(TARGET_ARCH_ABI)/libcurl.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../libcurl/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := blake2
LOCAL_SRC_FILES := $(LOCAL_PATH)/../blake2/obj/local/$(TARGET_ARCH_ABI)/libblake2.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../blake2
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := crsync
LOCAL_SRC_FILES := crsync.c onepiece.c onepiece-jni.c ../extra/tpl.c
LOCAL_C_INCLUDES += ../extra
LOCAL_STATIC_LIBRARIES := curl blake2
LOCAL_CFLAGS += -DCURL_STATICLIB -std=c99 -fopenmp
LOCAL_LDLIBS += -lc -lz -llog
LOCAL_LDFLAGS += -fopenmp

include $(BUILD_SHARED_LIBRARY)
