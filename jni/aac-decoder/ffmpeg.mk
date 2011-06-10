LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE 			:= aacffmpeg
LOCAL_SRC_FILES 		:= aac-ffmpeg-decoder.c

ifeq ($(MMS_WMA),yes)
	LOCAL_SRC_FILES 	:= $(LOCAL_SRC_FILES) aac-ffmpeg-wma-decoder.c
endif

LOCAL_C_INCLUDES 		:= $(LOCAL_PATH)/../ffmpeg $(NDK_ROOT)/sources/cpufeatures
LOCAL_CFLAGS 			:= $(cflags_loglevels)
include $(BUILD_STATIC_LIBRARY)

include $(LOCAL_PATH)/../ffmpeg/Android.mk

