LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := ffmpeg
LOCAL_SRC_FILES := libavcodec/aacdec.c libavcodec/aactab.c libavutil/log.c libavutil/mem.c \
				libavcodec/aacsbr.c libavcodec/mdct.c libavcodec/mpeg4audio.c libavcodec/utils.c \
				libavcodec/bitstream.c libavcodec/dsputil.c libavutil/mathematics.c \
				libavcodec/aac_parser.c libavcodec/aacps.c libavcodec/fft.c libavcodec/avpacket.c \
				libavcodec/faanidct.c libavcodec/jrevdct.c libavcodec/simple_idct.c \
				libavcodec/arm/fft_init_arm.c \
				libavutil/avstring.c \
				libavcodec/audioconvert.c \
				libavcodec/aac_ac3_parser.c \
				libavcodec/parser.c \
				libavutil/error.c

ifeq ($(MMS_WMA),yes)
	LOCAL_SRC_FILES := $(LOCAL_SRC_FILES) \
				libavcodec/wma.c libavcodec/wmadec.c \
				libavcodec/options.c libavcodec/opt.c libavcodec/eval.c \
				libavformat/asf.c libavformat/asfdec.c libavformat/asfcrypt.c libavformat/riff.c \
				libavformat/utils.c libavformat/avio.c libavformat/aviobuf.c libavformat/options.c \
				libavformat/cutils.c libavformat/metadata.c libavformat/metadata_compat.c \
				libavformat/avlanguage.c \
				libavutil/rational.c libavutil/crc.c libavutil/rc4.c libavutil/des.c
endif

LOCAL_C_INCLUDES := $(LOCAL_PATH)/libavutil $(LOCAL_PATH)/libavcodec

# -DCONFIG_NETWORK : due to a bug in libavformat/utils.c we must provide it directly
LOCAL_CFLAGS := -DHAVE_CONFIG_H -DCONFIG_NETWORK

include $(BUILD_STATIC_LIBRARY)
