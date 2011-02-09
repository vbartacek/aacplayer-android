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

#include "aac-faad2-decoder.h"
#include "neaacdec.h"

#include <stdlib.h>
#include <string.h>
#include <android/log.h>

#define AACDW "AACDecoder4"

#ifdef FAAD2_ONLY
#define av_free free
#define av_mallocz malloc
#endif

typedef struct _aacd4 {
    NeAACDecHandle hAac;
    unsigned long samplerate;
    unsigned char channels;
    unsigned long bytesconsumed;
    unsigned long bytesleft;
    unsigned char *buffer;
    unsigned long bbsize;
} aacd4;


static void* aacd4_start(unsigned char *buffer, unsigned long buffer_size);
static void aacd4_stop(aacd4 *info);
static void aacd4_try_decode( aacd4 *info, NeAACDecFrameInfo *pframe, unsigned char **buffer,
                        jshort *jsamples, unsigned long samples_size );
static int aacd4_probe(unsigned char *buffer, int len);

static jclass AACInfo_jclass;
static jfieldID AACInfo_samplerate_jfieldID;
static jfieldID AACInfo_channels_jfieldID;


/**
 * Copies the rest of the previous input buffer to the beginning of the new array.
 */
static void* aacd4_prepare_buffer( aacd4 *info, jbyte *jbuffer, jint inOff, jint inLen )
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

        int pos = aacd4_probe( jbuffer, info->bytesleft );

        __android_log_print(ANDROID_LOG_DEBUG, AACDW, "prepare_buf() aac sync pos=%d", pos );

        jbuffer += pos;
        info->bytesleft -= pos;

        return jbuffer;
    }
}



/*
 * Class:     com_spoledge_aacplayer_FAADDecoder
 * Method:    nativeStart
 * Signature: (Ljava/nio/ByteBuffer;Lcom/spoledge/aacplayer/Decoder/Info;)I
 */
jint Java_com_spoledge_aacplayer_FAADDecoder_nativeStart
  (JNIEnv *env, jobject thiz, jobject inBuf, jint inOff, jint inLen, jobject aacInfo)
{
    jboolean isCopy;
    jbyte *jbuffer;

    if (AACInfo_jclass == NULL)
    {
        AACInfo_jclass = (jclass) (*env)->GetObjectClass( env, aacInfo );
        AACInfo_samplerate_jfieldID = (jfieldID) (*env)->GetFieldID( env, AACInfo_jclass, "sampleRate", "I");
        AACInfo_channels_jfieldID = (jfieldID) (*env)->GetFieldID( env, AACInfo_jclass, "channels", "I");
    }

    jbuffer = (jbyte*) (*env)->GetDirectBufferAddress( env, inBuf );
     __android_log_print(ANDROID_LOG_DEBUG, AACDW, "get array isCopy=%d", isCopy );


    if (jbuffer == NULL)
    {
        __android_log_print(ANDROID_LOG_ERROR, AACDW, "cannot acquire java array" );
        return 0;
    }

    unsigned char *buffer;
    buffer = (unsigned char*) jbuffer + inOff;

    aacd4 *info;
    info = aacd4_start( buffer, inLen );

    (*env)->SetIntField( env, aacInfo, AACInfo_samplerate_jfieldID, (jint) info->samplerate);
    (*env)->SetIntField( env, aacInfo, AACInfo_channels_jfieldID, (jint) info->channels);

    return (jint) info;
}


/*
 * Class:     com_spoledge_aacplayer_FAADDecoder
 * Method:    nativeDecode
 * Signature: (ILjava/nio/ByteBuffer;Ljava/nio/ShortBuffer;)I
 */
jint Java_com_spoledge_aacplayer_FAADDecoder_nativeDecode
  (JNIEnv *env, jobject thiz, jint jinfo, jobject inBuf, jint inOff, jint inLen, jobject outBuf, jint outLen)
{
    aacd4 *info;
    info = (aacd4*) jinfo;

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
    buffer = (unsigned char*) aacd4_prepare_buffer( info, jbuffer, inOff, inLen );

    NeAACDecFrameInfo frame;
    int totalSamples = 0;
    jshort *samples = jsamples;

    /*
    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "decode() BEFORE n_in[0]=%x, n_in[1]=%x, || buf[0]=%x, buf[1]=%x", *(jbuffer+inOff), *(jbuffer+inOff+1), *buffer, *(buffer+1));
    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "decode() BEFORE n_out_short[0]=%x", *samples);
    */

    do
    {
        aacd4_try_decode( info, &frame, &buffer, samples, outLen );

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

        if (frame.samples < 1) __android_log_print(ANDROID_LOG_WARN, AACDW, "NeAACDecDecode no samples produced" );

        samples += frame.samples;
        outLen -= frame.samples;
        totalSamples += frame.samples;
    } 
    while (info->bytesleft > 0
              && info->bytesleft > 2*frame.bytesconsumed
              && 2*frame.samples < outLen );


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
        //__android_log_print(ANDROID_LOG_INFO, AACDW, "decode() AFTER bytesleft=%d buf[0]=%x, buf[1]=%x", info->bytesleft, *(info->buffer), *(info->buffer+1));
    }

    return (jint) totalSamples;
}


/*
 * Class:     com_spoledge_aacplayer_FAADDecoder
 * Method:    nativeStop
 * Signature: (I)V
 */
void Java_com_spoledge_aacplayer_FAADDecoder_nativeStop
  (JNIEnv *env, jobject thiz, jint jinfo)
{
    aacd4_stop( (aacd4*)jinfo);
}



void* aacd4_start(unsigned char *buffer, unsigned long buffer_size)
{
    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "starting native service" );

    aacd4 *info;
    info = (aacd4*) malloc(sizeof(struct _aacd4));

    info->buffer = NULL;
    info->bbsize = 0;
    info->bytesconsumed = 0;
    info->bytesleft = 0;

    info->hAac = NeAACDecOpen();
    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "start() FAAD2 capabilities: %d", NeAACDecGetCapabilities());

    NeAACDecConfigurationPtr conf = NeAACDecGetCurrentConfiguration( info->hAac );
    conf->outputFormat = FAAD_FMT_16BIT;
    conf->downMatrix = 1;
//conf->useOldADTSFormat = 1;
    NeAACDecSetConfiguration( info->hAac, conf);

    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "start() buffer=%d size=%d", buffer, buffer_size );

    int pos = aacd4_probe( buffer, buffer_size );
    buffer += pos;
    buffer_size -= pos;
    info->bytesconsumed = pos;

    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "start() pos=%d, b1=%d, b2=%d", pos, buffer[0], buffer[1] );

    long err = NeAACDecInit( info->hAac, buffer, buffer_size, &info->samplerate, &info->channels );

    if (err < 0)
    {
        __android_log_print(ANDROID_LOG_ERROR, AACDW, "NeAACDecInit failed err=%d", err );
        aacd4_stop( info );
        info = NULL;
    }
    else
    {
        info->bytesconsumed += err;
        __android_log_print(ANDROID_LOG_DEBUG, AACDW, "start() bytesconsumed=%d", info->bytesconsumed );
    }

    return info;
}


void aacd4_stop( aacd4 *info )
{
    if (info == NULL) return;

    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "stopping native service" );

    if (info->hAac != NULL)
    {
        NeAACDecClose( info->hAac );
        info->hAac = NULL;
    }

    if (info->buffer != NULL)
    {
        free( info->buffer );
        info->buffer = NULL;
        info->bbsize = 0;
    }

    info->bytesleft = 0;

    free( info );
}


void aacd4_try_decode( aacd4 *info, NeAACDecFrameInfo *pframe, unsigned char **buffer,
                        jshort *jsamples, unsigned long samples_size  ) {
    int attempts = 10;
    jshort *ljsamples = jsamples;

    do {
        NeAACDecDecode2( info->hAac, pframe, *buffer, info->bytesleft, (void**)&jsamples, samples_size );

        if (ljsamples != jsamples) {
            __android_log_print(ANDROID_LOG_WARN, AACDW, "NeAACDecDecode CHANGE jsamples !!!");
        }

        if (pframe->error == 0) return;

        __android_log_print(ANDROID_LOG_ERROR, AACDW, "NeAACDecDecode bytesleft=%d, error: %s",
                    info->bytesleft,
                    NeAACDecGetErrorMessage(pframe->error));

        if (info->bytesleft > 1)
        {
            int pos = aacd4_probe( (*buffer)+1, info->bytesleft-1 );
            (*buffer) += pos+1;
            info->bytesleft -= pos+1;
        }
    } while (attempts-- > 0 && info->bytesleft > 1);
}


int aacd4_probe(unsigned char *buffer, int len)
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


