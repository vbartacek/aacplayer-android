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

#include "aac-decoder.h"
#include "neaacdec.h"

#include <stdlib.h>
#include <string.h>
#include <android/log.h>

#define AACDW "AACDecoder"

typedef struct _aacdw {
    NeAACDecHandle hAac;
    unsigned long samplerate;
    unsigned char channels;
    unsigned long bytesconsumed;
    unsigned long bytesleft;
    unsigned char *buffer;
    unsigned char *buffer_block;
    unsigned long bbsize;
    unsigned char *buffer_block2;
    unsigned long bbsize2;
} aacdw;


static void* aacdw_start(unsigned char *buffer, unsigned long buffer_size);
static void aacdw_stop(aacdw *info);
static void* aacdw_prepare_buffer( aacdw *info, jbyte *jbuffer, jint inOff, jint inLen );
static void aacdw_try_decode( aacdw *info, NeAACDecFrameInfo *pframe, unsigned char **buffer,
                        jshort *jsamples, unsigned long samples_size );
static int aacdw_probe(unsigned char *buffer, int len);

static jclass AACInfo_jclass;
static jfieldID AACInfo_samplerate_jfieldID;
static jfieldID AACInfo_channels_jfieldID;


/*
 * Class:     com_spoledge_aacplayer_AACDecoder
 * Method:    nativeStart
 * Signature: ([BIILcom/spoledge/aacplayer/AACDecoder/AACInfo;)J
 */
jint Java_com_spoledge_aacplayer_AACDecoder_nativeStart
  (JNIEnv *env, jobject thiz, jbyteArray inBuf, jint inOff, jint inLen, jobject aacInfo)
{
    jboolean isCopy;
    jbyte *jbuffer;

    if (AACInfo_jclass == NULL)
    {
        AACInfo_jclass = (jclass) (*env)->GetObjectClass( env, aacInfo );
        AACInfo_samplerate_jfieldID = (jfieldID) (*env)->GetFieldID( env, AACInfo_jclass, "samplerate", "I");
        AACInfo_channels_jfieldID = (jfieldID) (*env)->GetFieldID( env, AACInfo_jclass, "channels", "I");
    }

    jbuffer = (jbyte*) (*env)->GetByteArrayElements( env, inBuf, &isCopy );
     __android_log_print(ANDROID_LOG_DEBUG, AACDW, "get array isCopy=%d", isCopy );


    if (jbuffer == NULL)
    {
        __android_log_print(ANDROID_LOG_ERROR, AACDW, "cannot acquire java array" );
        return 0;
    }

    unsigned char *buffer;
    buffer = (unsigned char*) jbuffer + inOff;

    aacdw *info;
    info = aacdw_start( buffer, inLen );

    (*env)->SetIntField( env, aacInfo, AACInfo_samplerate_jfieldID, (jint) info->samplerate);
    (*env)->SetIntField( env, aacInfo, AACInfo_channels_jfieldID, (jint) info->channels);

    (*env)->ReleaseByteArrayElements( env, inBuf, jbuffer, JNI_ABORT ); // read-only

    return (jint) info;
}


/*
 * Class:     com_spoledge_aacplayer_AACDecoder
 * Method:    nativeDecode
 * Signature: (J[BII[B)I
 */
jint Java_com_spoledge_aacplayer_AACDecoder_nativeDecode
  (JNIEnv *env, jobject thiz, jint jinfo, jbyteArray inBuf, jint inOff, jint inLen, jshortArray outBuf, jint outLen)
{
    aacdw *info;
    info = (aacdw*) jinfo;

    if (info->bytesconsumed >= inLen)
    {
        __android_log_print(ANDROID_LOG_INFO, AACDW, "consumed all bytes in start()" );
        info->bytesconsumed = 0;
        return 0;
    }

    jboolean isCopy;
    jbyte *jbuffer;
    jbuffer = (jbyte*) (*env)->GetByteArrayElements( env, inBuf, &isCopy );

    if (jbuffer == NULL)
    {
        __android_log_print(ANDROID_LOG_ERROR, AACDW, "cannot acquire java array (byte)" );
        return -1;
    }

    jshort *jsamples;
    jsamples = (jshort*) (*env)->GetShortArrayElements( env, outBuf, &isCopy );

    if (jsamples == NULL)
    {
        __android_log_print(ANDROID_LOG_ERROR, AACDW, "cannot acquire java array (short)" );
        (*env)->ReleaseByteArrayElements( env, inBuf, jbuffer, JNI_ABORT ); // read-only
        return -1;
    }

    unsigned char *buffer;
    buffer = (unsigned char*) aacdw_prepare_buffer( info, jbuffer, inOff, inLen );

    NeAACDecFrameInfo frame;
    int totalSamples = 0;
    jshort *samples = jsamples;

    do
    {
        aacdw_try_decode( info, &frame, &buffer, samples, outLen );

        if (frame.error != 0) {
            (*env)->ReleaseShortArrayElements( env, outBuf, jsamples, JNI_ABORT );
            (*env)->ReleaseByteArrayElements( env, inBuf, jbuffer, JNI_ABORT ); // read-only

            return (jint)-1;
        }

//        __android_log_print(ANDROID_LOG_WARN, AACDW, "frame decoded bytesconsumed=%d, samples=%d", frame.bytesconsumed, frame.samples );

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


    if (info->buffer == NULL && info->bytesleft > 0)
    {
        info->buffer_block = (unsigned char*) malloc( inLen );
        info->bbsize = inLen;
        info->buffer = info->buffer_block + (info->bbsize - info->bytesleft);

        memcpy( info->buffer, jbuffer + (inOff + inLen - info->bytesleft), info->bytesleft);
    }
    else if (info->buffer != NULL) info->buffer = buffer;


    (*env)->ReleaseByteArrayElements( env, inBuf, jbuffer, JNI_ABORT ); // read-only
    (*env)->ReleaseShortArrayElements( env, outBuf, jsamples, 0 ); // commit and free

    // internal faad2 buffer - do not touch !
    //free( samples );

    return (jint) totalSamples;
}


/*
 * Class:     com_spoledge_aacplayer_AACDecoder
 * Method:    nativeStop
 * Signature: (J)V
 */
void Java_com_spoledge_aacplayer_AACDecoder_nativeStop
  (JNIEnv *env, jobject thiz, jint jinfo)
{
    aacdw_stop( (aacdw*)jinfo);
}



void* aacdw_start(unsigned char *buffer, unsigned long buffer_size)
{
    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "starting native service" );

    aacdw *info;
    info = (aacdw*) malloc(sizeof(struct _aacdw));

    info->buffer = NULL;
    info->buffer_block = NULL;
    info->buffer_block2 = NULL;
    info->bbsize = 0;
    info->bbsize2 = 0;
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

    int pos = aacdw_probe( buffer, buffer_size );
    buffer += pos;
    buffer_size -= pos;
    info->bytesconsumed = pos;

    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "start() pos=%d, b1=%d, b2=%d", pos, buffer[0], buffer[1] );

    long err = NeAACDecInit( info->hAac, buffer, buffer_size, &info->samplerate, &info->channels );

    if (err < 0)
    {
        __android_log_print(ANDROID_LOG_ERROR, AACDW, "NeAACDecInit failed err=%d", err );
        aacdw_stop( info );
        info = NULL;
    }
    else
    {
        info->bytesconsumed += err;
        __android_log_print(ANDROID_LOG_DEBUG, AACDW, "start() bytesconsumed=%d", info->bytesconsumed );
    }

    return info;
}


void aacdw_stop( aacdw *info )
{
    if (info == NULL) return;

    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "stopping native service" );

    if (info->hAac != NULL)
    {
        NeAACDecClose( info->hAac );
        info->hAac = NULL;
    }

    if (info->buffer_block != NULL)
    {
        free( info->buffer_block );
        info->buffer_block = NULL;
        info->bbsize = 0;
    }

    if (info->buffer_block2 != NULL)
    {
        free( info->buffer_block2 );
        info->buffer_block = NULL;
        info->bbsize2 = 0;
    }

    info->buffer = NULL;
    info->bytesleft = 0;

    free( info );
}


void* aacdw_prepare_buffer( aacdw *info, jbyte *jbuffer, jint inOff, jint inLen )
{
    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "prepare_buf() inLen=%d, info->bytesleft=%d", inLen, info->bytesleft );

    if (info->buffer != NULL)
    {
        int newlen = info->bytesleft + inLen;

        if (info->bbsize2 < newlen) 
        {
            if (info->buffer_block2 != NULL) free( info->buffer_block2 );

            int realsize = newlen + 500; // avoid realocating by one or two bytes only

            info->buffer_block2 = (unsigned char*) malloc( realsize );
            info->bbsize2 = realsize;
        }

        if (info->bytesleft != 0) memcpy( info->buffer_block2, info->buffer, info->bytesleft );

        memcpy( info->buffer_block2 + info->bytesleft, jbuffer + inOff, inLen );

        info->buffer = info->buffer_block;
        info->buffer_block = info->buffer_block2;
        info->buffer_block2 = info->buffer;
        info->buffer = info->buffer_block;

        int tmp;
        tmp = info->bbsize;
        info->bbsize = info->bbsize2;
        info->bbsize2 = tmp;

        info->bytesleft += inLen;

        return info->buffer;
    }
    else {
        jbuffer += inOff + info->bytesconsumed;

        info->bytesleft = inLen - info->bytesconsumed;
        info->bytesconsumed = 0;

        int pos = aacdw_probe( jbuffer, info->bytesleft );

        __android_log_print(ANDROID_LOG_DEBUG, AACDW, "prepare_buf() aac sync pos=%d", pos );

        jbuffer += pos;
        info->bytesleft -= pos;

        return jbuffer;
    }
}


void aacdw_try_decode( aacdw *info, NeAACDecFrameInfo *pframe, unsigned char **buffer,
                        jshort *jsamples, unsigned long samples_size  ) {
    int attempts = 10;

    do {
        NeAACDecDecode2( info->hAac, pframe, *buffer, info->bytesleft, (void**)&jsamples, samples_size );

        if (pframe->error == 0) return;

        __android_log_print(ANDROID_LOG_ERROR, AACDW, "NeAACDecDecode bytesleft=%d, error: %s",
                    info->bytesleft,
                    NeAACDecGetErrorMessage(pframe->error));

        if (info->bytesleft > 1)
        {
            int pos = aacdw_probe( (*buffer)+1, info->bytesleft-1 );
            (*buffer) += pos+1;
            info->bytesleft -= pos+1;
        }
    } while (attempts-- > 0 && info->bytesleft > 1);
}


int aacdw_probe(unsigned char *buffer, int len)
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

