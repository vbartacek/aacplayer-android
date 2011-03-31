LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE 			:= aacffmpeg
LOCAL_SRC_FILES 		:= aac-ffmpeg-decoder.c
LOCAL_C_INCLUDES 		:= $(LOCAL_PATH)/../ffmpeg $(NDK_ROOT)/sources/cpufeatures
LOCAL_STATIC_LIBRARIES 	:= ffmpeg cpufeatures
LOCAL_LDLIBS 			:= -llog
include $(BUILD_SHARED_LIBRARY)

include $(LOCAL_PATH)/../ffmpeg/Android.mk

