LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

AAC_DECODER_FEATURES =
needs_mks = $(LOCAL_PATH)/array.mk
needs_cpufeatures =


# Decoder: FAAD2
ifneq ($(findstring faad2,$(AAC_DECODERS)),)
	AAC_DECODER_FEATURES	+= FAAD2
	needs_mks 				+= $(LOCAL_PATH)/faad2.mk
endif

# Decoder: FFMPEG
ifneq ($(findstring ffmpeg,$(AAC_DECODERS)),)
#	needs_cpufeatures 		= cpufeatures
	AAC_DECODER_FEATURES	+= FFMPEG
	needs_mks 				+= $(LOCAL_PATH)/ffmpeg.mk
	ifeq ($(MMS_WMA),yes)
		AAC_DECODER_FEATURES	+= FFMPEG_WMA
	endif
else ifeq ($(MMS_WMA),yes)
	AAC_DECODER_FEATURES	+= FFMPEG_WMA
	needs_mks 				+= $(LOCAL_PATH)/ffmpeg.mk
endif

# Decoder: OpenCORE
ifneq ($(findstring opencore-aacdec,$(AAC_DECODERS)),)
	AAC_DECODER_FEATURES	+= OPENCORE
	needs_mks 				+= $(LOCAL_PATH)/opencore-aacdec.mk
endif


# Build components:
include $(needs_mks)


# Build cpufeatures if needed
ifneq ($(needs_cpufeatures),)
	include $(NDK_ROOT)/sources/cpufeatures/Android.mk
endif

