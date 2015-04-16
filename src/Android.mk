LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := curl
LOCAL_SRC_FILES := $(LOCAL_PATH)/../libcurl/lib/$(TARGET_ARCH_ABI)/libcurl.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../libcurl/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := crsync
LOCAL_SRC_FILES := tpl.c blake2b-ref.c crsync.c com_shaddock_libcurl_Curl.c
LOCAL_STATIC_LIBRARIES := curl
LOCAL_CFLAGS += -DCURL_STATICLIB -std=c99
LOCAL_LDLIBS := -lc -lz -llog
include $(BUILD_SHARED_LIBRARY)
