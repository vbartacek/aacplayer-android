/*
** AACDecoder - Freeware Advanced Audio (AAC) Decoder for Android
** Copyright (C) 2010 Spolecne s.r.o., http://www.spoledge.com
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**/

#include <stdlib.h>
#include <string.h>
#include <android/log.h>
#include <cpu-features.h>

#include "libavcodec/avcodec.h"
#include "libavcodec/aac_parser.h"
#include "libavcodec/get_bits.h"
#include "libavcodec/mpeg4audio.h"
#include "libavutil/mem.h"
#include "libavutil/log.h"

#include "aac-ffmpeg-decoder.h"

#define AACDW "AACDecoder3"

typedef struct _AACDec3Info {
    AVCodecContext *avctx;
    AVPacket *avpkt;
    unsigned long samplerate;
    unsigned char channels;
    unsigned long bytesconsumed;
    unsigned long bytesleft;
    unsigned char *buffer;
    unsigned int bbsize;
} AACDec3Info;


typedef struct _FrameInfo {
    unsigned long bytesconsumed;
    int samples;
    unsigned int error;
} FrameInfo;

extern AVCodec aac_decoder;

#define DEBUG(x) \
    __android_log_print(ANDROID_LOG_DEBUG, AACDW, x );

#define DEBUG2(x,y) \
    __android_log_print(ANDROID_LOG_DEBUG, AACDW, x, y );

static jclass AACInfo_jclass;
static jfieldID AACInfo_samplerate_jfieldID;
static jfieldID AACInfo_channels_jfieldID;


static void aacd3_cpufeatures()
{
    uint64_t features = android_getCpuFeatures();
    int armv7 = features & ANDROID_CPU_ARM_FEATURE_ARMv7 ? 1 : 0;
    int neon = features & ANDROID_CPU_ARM_FEATURE_NEON ? 1 : 0;

    __android_log_print(ANDROID_LOG_INFO, AACDW, "CPU FEATURES %s %s", armv7 ? "ARMv7" : "-", neon ? "NEON" : "-");
}


static const char *aacd3_log_name(void *ctx)
{
    return "AACDec";
}


/**
 * Creates a new AVCodecContext.
 */
static AVCodecContext* aacd3_create_avctx( AVCodec *codec, AACDec3Info *info ) 
{
    AVCodecContext *avctx = (AVCodecContext*) av_mallocz( sizeof(AVCodecContext));

    AVClass *avcls = (AVClass*) av_mallocz( sizeof(AVClass));
    avcls->class_name = "AVCodecContext";
    avcls->item_name = aacd3_log_name;

    avctx->av_class = avcls;
    avctx->codec = codec;
    memcpy( avctx->codec_name, codec->name, strlen( codec->name ));
    avctx->codec_type = codec->type;
    avctx->codec_id = codec->id;
    avctx->priv_data = (void*) av_mallocz( codec->priv_data_size );

    avctx->sample_rate = info->samplerate;
    avctx->channels = info->channels;
    avctx->extradata_size = 0;

    return avctx;
}


/**
 * Destroys AVCodecContext.
 */
static void aacd3_destroy_avctx( AVCodecContext *avctx)
{
    if (avctx->av_class) av_free( (void*) avctx->av_class );
    av_free( avctx );
}


/**
 * Creates a new AVPacket.
 */
static AVPacket* aacd3_create_avpkt()
{
    AVPacket *avpkt = (AVPacket*) av_mallocz( sizeof(AVPacket));
    avpkt->data = NULL;
    avpkt->size = 0;

    return avpkt;
}


/**
 * Destroys AVPacket.
 */
static void aacd3_destroy_avpkt( AVPacket *avpkt)
{
    av_free( avpkt );
}


/**
 * Probes the stream and moves the pointer to the start of the next frame.
 */
static int aacd3_probe(unsigned char *buffer, int len)
{
  int i = 0, pos = 0;
  while (i <= len-4)
  {
      if(
       ((buffer[i] == 0xff) && ((buffer[i+1] & 0xf6) == 0xf0)) ||
       (buffer[i] == 'A' && buffer[i+1] == 'D' && buffer[i+2] == 'I' && buffer[i+3] == 'F')
    ) {
      pos = i;
      break;
    }
    i++;
  }

  return pos;
}


static void aacd3_parse_header(unsigned char *buffer, unsigned long buffer_size, AACDec3Info *info ) 
{
    GetBitContext bits;
    AACADTSHeaderInfo hdr;

    init_get_bits(&bits, buffer, AAC_ADTS_HEADER_SIZE * 8);

    if (ff_aac_parse_header(&bits, &hdr) < 0) {
        DEBUG( "ADTS parsing failed." );

        info->samplerate = -1;
        info->channels = -1;

        return;
    }

    /*
    hdr_info->sample_rate = hdr.sample_rate;
    hdr_info->channels    = ff_mpeg4audio_channels[hdr.chan_config];
    hdr_info->samples     = hdr.samples;
    hdr_info->bit_rate    = hdr.bit_rate;
    */

    info->samplerate = hdr.sample_rate;
    info->channels = ff_mpeg4audio_channels[hdr.chan_config];
}


/**
 * Starts the decoder.
 */
static void* aacd3_start(unsigned char *buffer, unsigned long buffer_size)
{
    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "starting native service" );

    av_log_set_level( AV_LOG_DEBUG );

    aacd3_cpufeatures();

    AACDec3Info *info;
    info = (AACDec3Info*) av_mallocz(sizeof(struct _AACDec3Info));

    info->avctx = NULL;
    info->avpkt = NULL;
    info->buffer = NULL;
    info->bbsize = 0;
    info->bytesconsumed = 0;
    info->bytesleft = 0;

    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "start() buffer=%d size=%d", buffer, buffer_size );

    int pos = aacd3_probe( buffer, buffer_size );
    buffer += pos;
    buffer_size -= pos;
    info->bytesconsumed = pos;

    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "start() pos=%d, b1=%d, b2=%d", pos, buffer[0], buffer[1] );

    aacd3_parse_header( buffer, buffer_size, info );

    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "start() sample rate=%d", info->samplerate );

    DEBUG( "start(): starting FFMPEG" );
    //DEBUG ONLY
    info->channels=2;
    info->samplerate=44100;
    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "start() FORCED sample rate=%d, channels=%d", info->samplerate, info->channels );
    info->avctx = aacd3_create_avctx( &aac_decoder, info );

    av_log( info->avctx, AV_LOG_INFO, "Test of AV_LOG_INFO\n" );
    av_log( info->avctx, AV_LOG_DEBUG, "Test of AV_LOG_DEBUG\n" );
    av_log( info->avctx, AV_LOG_VERBOSE, "Test of AV_LOG_VERBOSE\n" );

    (*aac_decoder.init)( info->avctx );
    DEBUG( "start() FFMPEG started" );

    info->avpkt = aacd3_create_avpkt();

    return info;
}


/**
 * Stops the decoder.
 */
static void aacd3_stop( AACDec3Info *info )
{
    if (info == NULL) return;

    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "stopping native service" );

    if (info->avctx != NULL)
    {
        aacd3_destroy_avctx( info->avctx );
        info->avctx = NULL;
    }

    if (info->avpkt != NULL)
    {
        aacd3_destroy_avpkt( info->avpkt );
        info->avpkt = NULL;
    }

    if (info->buffer != NULL)
    {
        av_free( info->buffer );
        info->buffer = NULL;
        info->bbsize = 0;
    }

    info->bytesleft = 0;

    av_free( info );
}


/**
 * Copies the rest of the previous input buffer to the beginning of the new array.
 */
static void* aacd3_prepare_buffer( AACDec3Info *info, jbyte *jbuffer, jint inOff, jint inLen )
{
    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "prepare_buf() inOff=%d, inLen=%d, info->bytesleft=%d", inOff, inLen, info->bytesleft );

    if (info->buffer != NULL)
    {
        if (inOff < info->bytesleft)
        {
            __android_log_print(ANDROID_LOG_ERROR, AACDW, "prepare_buf() cannot insert bytes to the buffer: inOff=%d, info->bytesleft=%d", inOff, info->bytesleft );

            info->bytesleft = inLen;

            return jbuffer + inOff;
        }
        else 
        {
            jbuffer += inOff - info->bytesleft;

            if (info->bytesleft > 0) memcpy( jbuffer, info->buffer, info->bytesleft );

            info->bytesleft += inLen;

            return jbuffer;
        }
    }
    else 
    {
        jbuffer += inOff + info->bytesconsumed;

        info->bytesleft = inLen - info->bytesconsumed;
        info->bytesconsumed = 0;

        int pos = aacd3_probe( jbuffer, info->bytesleft );

        __android_log_print(ANDROID_LOG_DEBUG, AACDW, "prepare_buf() aac sync pos=%d", pos );

        jbuffer += pos;
        info->bytesleft -= pos;

        return jbuffer;
    }
}



static void aacd3_try_decode( AACDec3Info *info, FrameInfo *frame, unsigned char **buffer,
                        jshort *jsamples, unsigned long samples_size  )
{
    int attempts = 10;

    AVCodecContext *avctx = info->avctx;
    AVPacket *avpkt = info->avpkt;
    AVCodec *codec = avctx->codec;

    do {
        avpkt->data = *buffer;
        avpkt->size = info->bytesleft;
        avpkt->pos = 0;

        frame->samples = 0;

        // aac_decode_frame
        int consumed = (*codec->decode)( avctx, jsamples, (int*) &samples_size, avpkt );

        if (consumed > 0) {
            frame->error = 0;
            frame->bytesconsumed = consumed;
            frame->samples = avctx->frame_size * avctx->channels;

            return;
        }

        __android_log_print(ANDROID_LOG_ERROR, AACDW, "Decode bytesleft=%d, error: %d",
                    info->bytesleft, consumed );

        frame->error = consumed;
        frame->bytesconsumed = 0;

        if (info->bytesleft > 1)
        {
            int pos = aacd3_probe( (*buffer)+1, info->bytesleft-1 );
            (*buffer) += pos+1;
            info->bytesleft -= pos+1;
        }
    } while (attempts-- > 0 && info->bytesleft > 1);
}



/*
 * Class:     com_spoledge_aacplayer_FFMPEGDecoder
 * Method:    nativeStart
 * Signature: (Ljava/nio/ByteBuffer;Lcom/spoledge/aacplayer/Decoder/Info;)I
 */
JNIEXPORT jint JNICALL Java_com_spoledge_aacplayer_FFMPEGDecoder_nativeStart
  (JNIEnv *env, jobject thiz, jobject inBuf, jint inOff, jint inLen, jobject aacInfo)
{
    DEBUG2("starting native service - codec '%s'", aac_decoder.name );

    jbyte *jbuffer;

    if (AACInfo_jclass == NULL)
    {
        AACInfo_jclass = (jclass) (*env)->GetObjectClass( env, aacInfo );
        AACInfo_samplerate_jfieldID = (jfieldID) (*env)->GetFieldID( env, AACInfo_jclass, "sampleRate", "I");
        AACInfo_channels_jfieldID = (jfieldID) (*env)->GetFieldID( env, AACInfo_jclass, "channels", "I");
    }

    jbuffer = (jbyte*) (*env)->GetDirectBufferAddress( env, inBuf );

    if (jbuffer == NULL)
    {
        __android_log_print(ANDROID_LOG_ERROR, AACDW, "cannot acquire direct input buffer" );
        return 0;
    }

    unsigned char *buffer;
    buffer = (unsigned char*) jbuffer + inOff;

    AACDec3Info *info;
    info = aacd3_start( buffer, inLen );

    (*env)->SetIntField( env, aacInfo, AACInfo_samplerate_jfieldID, (jint) info->samplerate);
    (*env)->SetIntField( env, aacInfo, AACInfo_channels_jfieldID, (jint) info->channels);

    return (jint) info;
}


/*
 * Class:     com_spoledge_aacplayer_FFMPEGDecoder
 * Method:    nativeDecode
 * Signature: (ILjava/nio/ByteBuffer;Ljava/nio/ShortBuffer;)I
 */
JNIEXPORT jint JNICALL Java_com_spoledge_aacplayer_FFMPEGDecoder_nativeDecode
  (JNIEnv *env, jobject thiz, jint jinfo, jobject inBuf, jint inOff, jint inLen, jobject outBuf, jint outLen)
{
    AACDec3Info *info;
    info = (AACDec3Info*) jinfo;

    if (info->bytesconsumed >= inLen)
    {
        __android_log_print(ANDROID_LOG_INFO, AACDW, "consumed all bytes in start()" );
        info->bytesconsumed = 0;
        return 0;
    }

    jbyte *jbuffer;
    jbuffer = (jbyte*) (*env)->GetDirectBufferAddress( env, inBuf );

    if (jbuffer == NULL)
    {
        __android_log_print(ANDROID_LOG_ERROR, AACDW, "cannot acquire direct input buffer" );
        return 0;
    }

    jshort *jsamples;
    jsamples = (jshort*) (*env)->GetDirectBufferAddress( env, outBuf );

    if (jsamples == NULL)
    {
        __android_log_print(ANDROID_LOG_ERROR, AACDW, "cannot acquire direct output buffer" );
        return -1;
    }

    unsigned char *buffer;
    buffer = (unsigned char*) aacd3_prepare_buffer( info, jbuffer, inOff, inLen );

    FrameInfo frame;
    int totalSamples = 0;
    jshort *samples = jsamples;

    do
    {
        aacd3_try_decode( info, &frame, &buffer, samples, outLen );

        if (frame.error != 0) {
            __android_log_print(ANDROID_LOG_ERROR, AACDW, "Could not decode frame !!!" );
            return (jint)-1;
        }

//        __android_log_print(ANDROID_LOG_INFO, AACDW, "frame decoded bytesconsumed=%d, samples=%d", frame.bytesconsumed, frame.samples );

        // swap bytes:
        int cnt = frame.samples;
        jshort *ss = samples;
        while (cnt-- > 0) {
            uint16_t s = (uint16_t) *ss;
            *(ss++) = (s >> 8) | (s << 8);
        }

        info->bytesleft -= frame.bytesconsumed;
        buffer += frame.bytesconsumed;

        if (frame.samples < 1) __android_log_print(ANDROID_LOG_WARN, AACDW, "Decode no samples produced" );

        samples += frame.samples;
        outLen -= frame.samples;
        totalSamples += frame.samples;
    } 
    while (info->bytesleft > 0
              && info->bytesleft > 2*frame.bytesconsumed
              && frame.samples <= outLen );


    // remember the rest of the input buffer:
    if (info->bytesleft > 0)
    {
        if (info->bytesleft > info->bbsize)
        {
            if (info->buffer != NULL) av_free( info->buffer );
            info->buffer = NULL;
            info->buffer = (unsigned char*) av_mallocz( info->bytesleft );
            info->bbsize = info->bytesleft;
        }

        memcpy( info->buffer, buffer, info->bytesleft);
    }

    return (jint) totalSamples;
}


/*
 * Class:     com_spoledge_aacplayer_FFMPEGDecoder
 * Method:    nativeStop
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_spoledge_aacplayer_FFMPEGDecoder_nativeStop
  (JNIEnv *env, jobject thiz, jint jinfo)
{
    aacd3_stop( (AACDec3Info*)jinfo);
}



