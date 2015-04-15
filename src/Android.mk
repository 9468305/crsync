LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := curl
LOCAL_SRC_FILES := $(addprefix $(LOCAL_PATH)/../libcurl/lib/,$(TARGET_ARCH_ABI)/libcurl.a)
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../libcurl/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := tpl.c \
    blake2b-ref.c \
    crsync.c

LOCAL_CFLAGS += -DCURL_STATICLIB -std=c99

LOCAL_MODULE := crsync
LOCAL_STATIC_LIBRARIES := curl
LOCAL_LDLIBS := -lc -lz
include $(BUILD_SHARED_LIBRARY)
