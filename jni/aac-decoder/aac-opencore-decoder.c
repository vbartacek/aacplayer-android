/*
** AACDecoder - Freeware Advanced Audio (AAC) Decoder for Android
** Copyright (C) 2011 Spolecne s.r.o., http://www.spoledge.com
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

#include "aac-opencore-decoder.h"
#include "pvmp4audiodecoder_api.h"
#include "e_tmp4audioobjecttype.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <android/log.h>

#define AACDW "AACDec[OpenCORE]"

#ifdef OPENCORE_ONLY
#define av_free free
#define av_mallocz malloc
#endif

typedef struct _aacd5 {
    tPVMP4AudioDecoderExternal *pExt;
    void *pMem;
    unsigned long samplerate;
    unsigned char channels;
    unsigned long bytesconsumed;
    unsigned long bytesleft;
    unsigned char *buffer;
    unsigned long bbsize;
} aacd5;


static void* aacd5_start(unsigned char *buffer, unsigned long buffer_size);
static void aacd5_stop(aacd5 *info);
static int32_t aacd5_try_decode( aacd5 *info, unsigned char **buffer, jshort *jsamples );
static int aacd5_probe(unsigned char *buffer, int len);

static jclass AACInfo_jclass;
static jfieldID AACInfo_samplerate_jfieldID;
static jfieldID AACInfo_channels_jfieldID;

/*
 * Debugging
 * */
void print_bytes(uint8_t *bytes, int len) {
    int i;
    int count;
    int done = 0;
    char out[128];
    memset( out, 0, 128);

    while (len > done) {
        if (len-done > 32){
            count = 32;
        } else {
            count = len-done;
        }

        for (i=0; i<count; i++) {
            sprintf( out+i*3, "%02x ", bytes[done+i]);
        }
        out[count*3] = 0;

        __android_log_print(ANDROID_LOG_DEBUG, AACDW, "%s", out );

        done += count;
    }
}


/**
 * Copies the rest of the previous input buffer to the beginning of the new array.
 */
static void* aacd5_prepare_buffer( aacd5 *info, jbyte *jbuffer, jint inOff, jint inLen )
{
//    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "prepare_buf() inOff=%d, inLen=%d, info->bytesleft=%d", inOff, inLen, info->bytesleft );

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

        int pos = aacd5_probe( jbuffer, info->bytesleft );

        __android_log_print(ANDROID_LOG_DEBUG, AACDW, "prepare_buf() aac sync pos=%d", pos );

        jbuffer += pos;
        info->bytesleft -= pos;

        return jbuffer;
    }
}



/*
 * Class:     com_spoledge_aacplayer_OpenCOREDecoder
 * Method:    nativeStart
 * Signature: (Ljava/nio/ByteBuffer;Lcom/spoledge/aacplayer/Decoder/Info;)I
 */
jint Java_com_spoledge_aacplayer_OpenCOREDecoder_nativeStart
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

    aacd5 *info;
    info = aacd5_start( buffer, inLen );

    (*env)->SetIntField( env, aacInfo, AACInfo_samplerate_jfieldID, (jint) info->samplerate);
    (*env)->SetIntField( env, aacInfo, AACInfo_channels_jfieldID, (jint) info->channels);

    return (jint) info;
}


/*
 * Class:     com_spoledge_aacplayer_OpenCOREDecoder
 * Method:    nativeDecode
 * Signature: (ILjava/nio/ByteBuffer;Ljava/nio/ShortBuffer;)I
 */
jint Java_com_spoledge_aacplayer_OpenCOREDecoder_nativeDecode
  (JNIEnv *env, jobject thiz, jint jinfo, jobject inBuf, jint inOff, jint inLen, jobject outBuf, jint outLen)
{
    aacd5 *info;
    info = (aacd5*) jinfo;

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
    buffer = (unsigned char*) aacd5_prepare_buffer( info, jbuffer, inOff, inLen );

    tPVMP4AudioDecoderExternal *pExt = info->pExt;
    int totalSamples = 0;
    jshort *samples = jsamples;

    /*
    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "decode() BEFORE n_in[0]=%x, n_in[1]=%x, || buf[0]=%x, buf[1]=%x", *(jbuffer+inOff), *(jbuffer+inOff+1), *buffer, *(buffer+1));
    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "decode() BEFORE n_out_short[0]=%x", *samples);
    */

    do
    {
        int32_t status = aacd5_try_decode( info, &buffer, samples );

        if (status != SUCCESS) {
            __android_log_print(ANDROID_LOG_ERROR, AACDW, "Could not decode frame !!!" );
            return (jint)-1;
        }

        int produced = pExt->frameLength * pExt->desiredChannels;
        if (2 == pExt->aacPlusUpsamplingFactor) produced *= 2;

        // swap bytes:
        int cnt = produced;
        jshort *ss = samples;
        while (cnt-- > 0) {
            uint16_t s = (uint16_t) *ss;
            *(ss++) = (s >> 8) | (s << 8);
        }

//        print_bytes( samples, produced );
//        __android_log_print(ANDROID_LOG_INFO, AACDW, "frame decoded bytesconsumed=%d, samples=%d", pExt->inputBufferUsedLength, produced );

        info->bytesleft -= pExt->inputBufferUsedLength;
        buffer += pExt->inputBufferUsedLength;

        samples += produced;
        outLen -= produced;
        totalSamples += produced;
    } 
    while (info->bytesleft > 0
              && info->bytesleft > 2*pExt->inputBufferUsedLength
              && 4096 <= outLen );


    // remember the rest of the input buffer:
    if (info->bytesleft > 0)
    {
        if (info->bytesleft > info->bbsize)
        {
            if (info->buffer != NULL) free( info->buffer );
            info->buffer = NULL;
            info->buffer = (unsigned char*) calloc( info->bytesleft, 1 );
            info->bbsize = info->bytesleft;
        }

        memcpy( info->buffer, buffer, info->bytesleft);
        //__android_log_print(ANDROID_LOG_INFO, AACDW, "decode() AFTER bytesleft=%d buf[0]=%x, buf[1]=%x", info->bytesleft, *(info->buffer), *(info->buffer+1));
    }

    return (jint) totalSamples;
}


/*
 * Class:     com_spoledge_aacplayer_OpenCOREDecoder
 * Method:    nativeStop
 * Signature: (I)V
 */
void Java_com_spoledge_aacplayer_OpenCOREDecoder_nativeStop
  (JNIEnv *env, jobject thiz, jint jinfo)
{
    aacd5_stop( (aacd5*)jinfo);
}



void* aacd5_start(unsigned char *buffer, unsigned long buffer_size)
{
    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "starting native service" );

    aacd5 *info;
    info = (aacd5*) malloc(sizeof(struct _aacd5));
    tPVMP4AudioDecoderExternal *pExt = malloc(sizeof(tPVMP4AudioDecoderExternal));

    info->buffer = NULL;
    info->bbsize = 0;
    info->bytesconsumed = 0;
    info->bytesleft = 0;

    info->pExt = pExt;
    info->pMem = malloc(PVMP4AudioDecoderGetMemRequirements());

    pExt->pInputBuffer              = buffer;
    pExt->inputBufferMaxLength      = buffer_size;
    pExt->desiredChannels           = 2;
    pExt->inputBufferCurrentLength  = 0;
    pExt->outputFormat              = OUTPUTFORMAT_16PCM_INTERLEAVED;
    pExt->repositionFlag            = TRUE;
    pExt->aacPlusEnabled            = TRUE;
    pExt->inputBufferUsedLength     = 0;
    pExt->remainderBits             = 0;
    pExt->frameLength               = 0;

    // we do not have output buffer yet - tmp buffers:
    pExt->pOutputBuffer             = calloc(4096, sizeof(int16_t));
    pExt->pOutputBuffer_plus        = pExt->pOutputBuffer + 2048;

    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "start() buffer=%d size=%d", buffer, buffer_size );

    Int err = PVMP4AudioDecoderInitLibrary(pExt, info->pMem);

    if (err)
    {
        __android_log_print(ANDROID_LOG_ERROR, AACDW, "PVMP4AudioDecoderInitLibrary failed err=%d", err );
        aacd5_stop( info );

        return NULL;
    }

    int pos = aacd5_probe( buffer, buffer_size );
    buffer += pos;
    buffer_size -= pos;
    info->bytesconsumed = pos;

    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "start() pos=%d, b1=%d, b2=%d", pos, buffer[0], buffer[1] );

    int32_t status;
    int frameDecoded = 0;

    /* pre-init search adts sync */
    while (pExt->frameLength == 0) {
        pExt->pInputBuffer = buffer;
        pExt->inputBufferCurrentLength = buffer_size;
        pExt->inputBufferUsedLength = 0;

        status = PVMP4AudioDecoderConfig(pExt, info->pMem);
        __android_log_print(ANDROID_LOG_DEBUG, AACDW, "start() Status[0]: %d", status );

        if (status != MP4AUDEC_SUCCESS) {
            status = PVMP4AudioDecodeFrame(pExt, info->pMem);
            __android_log_print(ANDROID_LOG_DEBUG, AACDW, "start() Status[1]: %d", status );

            pos = pExt->inputBufferUsedLength;
            buffer -= pos;
            buffer_size -= pos;
            info->bytesconsumed += pos;

            if (MP4AUDEC_SUCCESS == status) {
                __android_log_print(ANDROID_LOG_DEBUG, AACDW, "start() frameLength: %d\n", pExt->frameLength);
                frameDecoded = 1;
                continue;
            }
        }

        if (buffer_size <= 64) break;
    }

    if (!frameDecoded) status = PVMP4AudioDecodeFrame(pExt, info->pMem);

    if (status != MP4AUDEC_SUCCESS)
    {
        __android_log_print(ANDROID_LOG_ERROR, AACDW, "start() init failed status=%d", status );
        free( pExt->pOutputBuffer );
        aacd5_stop( info );
        return NULL;
    }

    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "start() bytesconsumed=%d", info->bytesconsumed );

    int streamType  = -1;

    if ((pExt->extendedAudioObjectType == MP4AUDIO_AAC_LC) ||
            (pExt->extendedAudioObjectType == MP4AUDIO_LTP))
    {
        streamType = AAC;
    }
    else if (pExt->extendedAudioObjectType == MP4AUDIO_SBR)
    {
        streamType = AACPLUS;
    }
    else if (pExt->extendedAudioObjectType == MP4AUDIO_PS)
    {
        streamType = ENH_AACPLUS;
    }

    __android_log_print(ANDROID_LOG_INFO, AACDW, "start() streamType=%d", streamType );

    if ((AAC == streamType) && (2 == pExt->aacPlusUpsamplingFactor))
    {
        __android_log_print(ANDROID_LOG_INFO, AACDW, "start() DisableAacPlus" );
        PVMP4AudioDecoderDisableAacPlus(pExt, info->pMem);
    }

    info->samplerate = pExt->samplingRate;
    info->channels = pExt->desiredChannels;

    free( pExt->pOutputBuffer );

    return info;
}


void aacd5_stop( aacd5 *info )
{
    if (info == NULL) return;

    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "stopping native service" );

    if (info->pMem != NULL)
    {
        free( info->pMem );
        info->pMem = NULL;
    }

    if (info->pExt != NULL)
    {
        free( info->pExt );
        info->pExt = NULL;
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


int32_t aacd5_try_decode( aacd5 *info, unsigned char **buffer, jshort *jsamples ) {
    int attempts = 10;
    tPVMP4AudioDecoderExternal *pExt = info->pExt;
    int32_t status;

    pExt->pOutputBuffer = jsamples;
    pExt->pOutputBuffer_plus = jsamples+2048;

    do {
        pExt->pInputBuffer              = *buffer;
        pExt->inputBufferMaxLength      = info->bytesleft;
        pExt->inputBufferCurrentLength  = info->bytesleft;
        pExt->inputBufferUsedLength     = 0;

        status = PVMP4AudioDecodeFrame( pExt, info->pMem );

        if (status == MP4AUDEC_SUCCESS || status == SUCCESS) return SUCCESS;

        __android_log_print(ANDROID_LOG_ERROR, AACDW, "decode() bytesleft=%d, status=%d", info->bytesleft, status );

        if (info->bytesleft > 1)
        {
            int pos = aacd5_probe( (*buffer)+1, info->bytesleft-1 );
            (*buffer) += pos+1;
            info->bytesleft -= pos+1;
        }
    } while (attempts-- > 0 && info->bytesleft > 1);

    return status;
}


int aacd5_probe(unsigned char *buffer, int len)
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


