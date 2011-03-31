LOCAL_PATH := $(call my-dir)
LOCAL_PATH_AAC_DEC := $(LOCAL_PATH)

include $(CLEAR_VARS)

AAC_FEATURES =
needs_mks =
needs_cpufeatures =


# Decoder: FAAD2
ifneq ($(findstring faad2,$(AAC_DECODERS)),)
	AAC_FEATURES		+= FAAD2
	needs_mks 			+= $(LOCAL_PATH)/faad2.mk
endif

# Decoder: FFMPEG
ifneq ($(findstring ffmpeg,$(AAC_DECODERS)),)
	needs_cpufeatures 	= cpufeatures
	AAC_FEATURES		+= FFMPEG
	needs_mks 			+= $(LOCAL_PATH)/ffmpeg.mk
endif

# Decoder: OpenCORE
ifneq ($(findstring opencore-aacdec,$(AAC_DECODERS)),)
	AAC_FEATURES		+= OPENCORE
	needs_mks 			+= $(LOCAL_PATH)/opencore-aacdec.mk
endif


# Build final shared library
LOCAL_MODULE    		:= AACDecoder
LOCAL_SRC_FILES 		:= aac-decoder.c
LOCAL_CFLAGS			:= -DAAC_FEATURES='0x00$(foreach feature,$(AAC_FEATURES), | com_spoledge_aacplayer_Decoder_DECODER_$(feature))' $(foreach feature,$(AAC_FEATURES),-DAAC_FEATURE_$(feature))
LOCAL_LDLIBS 			:= -llog
include $(BUILD_SHARED_LIBRARY)


# Build components:
include $(needs_mks)


# Build cpufeatures if needed
ifneq ($(needs_cpufeatures),)
	include $(NDK_ROOT)/sources/cpufeatures/Android.mk
endif

