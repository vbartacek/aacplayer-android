LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := AACDecoder
LOCAL_SRC_FILES := aac-decoder.c aac-faad2-decoder.c aac-ffmpeg-decoder.c
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../faad2/include $(LOCAL_PATH)/../ffmpeg $(NDK_ROOT)/sources/cpufeatures

# Due to a bug in Android 1.5 we cannot have two shared libs:
#LOCAL_SHARED_LIBRARIES := faad2
LOCAL_STATIC_LIBRARIES := faad2 ffmpeg cpufeatures

LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)

include $(NDK_ROOT)/sources/cpufeatures/Android.mk
