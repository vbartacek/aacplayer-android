LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE 			:= aacopencore-aacdec
LOCAL_SRC_FILES 		:= aac-opencore-decoder.c
LOCAL_C_INCLUDES 		:= $(LOCAL_PATH)/../opencore-aacdec/include
LOCAL_CFLAGS 			:= $(cflags_loglevels)
include $(BUILD_STATIC_LIBRARY)

include $(LOCAL_PATH)/../opencore-aacdec/Android.mk

