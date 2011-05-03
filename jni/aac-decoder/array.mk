LOCAL_PATH := $(call my-dir)

cflags_array_features	:= -DAAC_ARRAY_FEATURES='0x00$(foreach feature,$(AAC_DECODER_FEATURES), | com_spoledge_aacplayer_Decoder_DECODER_$(feature))' $(foreach feature,$(AAC_DECODER_FEATURES),-DAAC_ARRAY_FEATURE_$(feature))
static_libs_array		:= $(foreach feature,$(AAC_DECODERS), aac$(feature) $(feature))

include $(CLEAR_VARS)
LOCAL_MODULE 			:= aacarray
LOCAL_SRC_FILES 		:= aac-common.c aac-array-common.c aac-array-decoder.c
LOCAL_CFLAGS 			:= $(cflags_array_features) $(cflags_loglevels)
LOCAL_LDLIBS 			:= -llog
LOCAL_STATIC_LIBRARIES 	:= $(static_libs_array)
include $(BUILD_SHARED_LIBRARY)

