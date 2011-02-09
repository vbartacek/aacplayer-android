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
				libavcodec/parser.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/libavutil $(LOCAL_PATH)/libavcodec

LOCAL_CFLAGS := -DHAVE_CONFIG_H 

include $(BUILD_STATIC_LIBRARY)
