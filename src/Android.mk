LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := curl
LOCAL_SRC_FILES := $(LOCAL_PATH)/../libcurl/lib/$(TARGET_ARCH_ABI)/libcurl.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../libcurl/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := crsync
LOCAL_SRC_FILES := crsync.c onepiece.c crsyncjni.c ../extra/tpl.c ../extra/blake2s-ref.c
LOCAL_C_INCLUDES += ../extra
LOCAL_STATIC_LIBRARIES := curl
LOCAL_CFLAGS += -DCURL_STATICLIB -std=c99
LOCAL_LDLIBS := -lc -lz -llog
include $(BUILD_SHARED_LIBRARY)
