LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

AAC_DECODER_FEATURES =
LOGLEVELS =
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
endif

# Decoder: OpenCORE
ifneq ($(findstring opencore-aacdec,$(AAC_DECODERS)),)
	AAC_DECODER_FEATURES	+= OPENCORE
	needs_mks 				+= $(LOCAL_PATH)/opencore-aacdec.mk
endif


# Loglevels
ifeq ($(LOGLEVEL),error)
	LOGLEVELS	+= ERROR
endif
ifeq ($(LOGLEVEL),warn)
	LOGLEVELS	+= ERROR WARN
endif
ifeq ($(LOGLEVEL),info)
	LOGLEVELS	+= ERROR WARN INFO
endif
ifeq ($(LOGLEVEL),debug)
	LOGLEVELS	+= ERROR WARN INFO DEBUG
endif
ifeq ($(LOGLEVEL),trace)
	LOGLEVELS	+= ERROR WARN INFO DEBUG TRACE
endif

cflags_loglevels	:= $(foreach ll,$(LOGLEVELS),-DAACD_LOGLEVEL_$(ll))


# Build components:
include $(needs_mks)


# Build cpufeatures if needed
ifneq ($(needs_cpufeatures),)
	include $(NDK_ROOT)/sources/cpufeatures/Android.mk
endif

