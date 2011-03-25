LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := AACDecoder
LOCAL_SRC_FILES := aac-opencore-decoder.c
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../opencore-aacdec/include $(NDK_ROOT)/sources/cpufeatures

# Due to a bug in Android 1.5 we cannot have two shared libs:
LOCAL_STATIC_LIBRARIES := opencore-aacdec cpufeatures

LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)

include $(NDK_ROOT)/sources/cpufeatures/Android.mk
