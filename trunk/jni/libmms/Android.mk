#
# Compile libmms
# 
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := mms
LOCAL_SRC_FILES := src/mms.c src/mmsh.c src/mmsx.c src/uri.c mms-inputstream.c
LOCAL_CFLAGS	:= -DHAVE_CONFIG_H $(cflags_loglevels)
LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_LDLIBS 	:= -llog

include $(BUILD_SHARED_LIBRARY)

