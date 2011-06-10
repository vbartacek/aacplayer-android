/*
** AACPlayer - Freeware Advanced Audio (AAC) Player for Android
** Copyright (C) 2011 Spolecne s.r.o., http://www.spoledge.com
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program. If not, see <http://www.gnu.org/licenses/>.
**/

#define AACD_MODULE "ArrayDecoder"

#include "aac-array-decoder.h"
#include "aac-array-common.h"

#include <string.h>

extern AACDDecoder aacd_faad_decoder;
extern AACDDecoder aacd_ffmpeg_decoder;
extern AACDDecoder aacd_ffmpeg_wma_decoder;
extern AACDDecoder aacd_opencore_decoder;


AACDDecoder* AACDDecoders[4] = {

#ifdef AAC_ARRAY_FEATURE_FAAD2
    &aacd_faad_decoder,
#else
    NULL,
#endif

#ifdef AAC_ARRAY_FEATURE_FFMPEG
    &aacd_ffmpeg_decoder,
#else
    NULL,
#endif

#ifdef AAC_ARRAY_FEATURE_OPENCORE
    &aacd_opencore_decoder,
#else
    NULL,
#endif

#ifdef AAC_ARRAY_FEATURE_FFMPEG_WMA
    &aacd_ffmpeg_wma_decoder
#else
    NULL
#endif

};


/****************************************************************************************************
 * FUNCTIONS
 ****************************************************************************************************/

static AACDDecoder* aacda_decoder( int decoder )
{
    if (!decoder) decoder = 1;

    int n = 0;

    while (!(decoder & 0x01))
    { 
        decoder >>= 1;
        n++;
    }

    return AACDDecoders[ n ];
}


/*
 * Class:     com_spoledge_aacplayer_ArrayDecoder
 * Method:    nativeGetFeatures
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_spoledge_aacplayer_ArrayDecoder_nativeGetFeatures
  (JNIEnv *env, jclass clazz)
{
    return AAC_ARRAY_FEATURES;
}


/*
 * Class:     com_spoledge_aacplayer_ArrayDecoder
 * Method:    nativeStart
 * Signature: (ILcom/spoledge/aacplayer/ArrayBufferReader;Lcom/spoledge/aacplayer/Decoder/Info;)I
 */
JNIEXPORT jint JNICALL Java_com_spoledge_aacplayer_ArrayDecoder_nativeStart
  (JNIEnv *env, jobject thiz, jint decoder, jobject jreader, jobject aacInfo)
{
    AACD_TRACE( "start() start" );
    AACDDecoder *dec = aacda_decoder( decoder );

    if (!dec)
    {
        AACD_ERROR( "start() decoder [%d] not supported", decoder );
        return 0;
    }

    AACDArrayInfo *ainfo = aacda_start( env, dec, jreader, aacInfo );

    if (!ainfo)
    {
        AACD_ERROR( "start() cannot initialize decoder - out-of-memory error ?" );
        return 0;
    }

    ainfo->env = env;

    AACD_TRACE( "start() calling read_buffer" );

    unsigned char* buffer = aacda_read_buffer( ainfo );
    unsigned long buffer_size = ainfo->cinfo.bytesleft;

    AACD_TRACE( "start() got %d bytes from read_buffer", buffer_size );

    int pos = ainfo->decoder->sync( buffer, buffer_size );
    AACD_TRACE( "start() sync returned %d", pos );

    if (pos < 0)
    {
        AACD_ERROR( "start() failed - ADTS sync word not found" );
        aacda_stop( ainfo );

        return 0;
    }

    buffer += pos;
    buffer_size -= pos;

    AACD_TRACE( "start() calling decoder->start()" );
    long err = ainfo->decoder->start( &ainfo->cinfo, ainfo->ext, buffer, buffer_size );

    if (err < 0)
    {
        AACD_ERROR( "start() failed err=%d", err );
        aacda_stop( ainfo );

        return 0;
    }

    // remember pointers for first decode round:
    if (!ainfo->cinfo.input_ctrl)
    {
        ainfo->cinfo.buffer = buffer + err;
        ainfo->cinfo.bytesleft = buffer_size - err;
    }

    AACD_DEBUG( "start() bytesleft=%d", ainfo->cinfo.bytesleft );

    aacd_start_info2java( env, &ainfo->cinfo, aacInfo );

    ainfo->env = NULL;

    AACD_TRACE( "nativeStart() stop" );

    return (jint) ainfo;
}


/*
 * Class:     com_spoledge_aacplayer_ArrayDecoder
 * Method:    nativeDecode
 * Signature: (I[SI)I
 */
JNIEXPORT jint JNICALL Java_com_spoledge_aacplayer_ArrayDecoder_nativeDecode
  (JNIEnv *env, jobject thiz, jint jinfo, jshortArray outBuf, jint outLen)
{
    AACDArrayInfo *ainfo = (AACDArrayInfo*) jinfo;
    ainfo->env = env;

    // prepare internal output buffer :
    jshort *jsamples = aacda_prepare_samples( ainfo, outLen );

    aacda_decode( ainfo, jsamples, outLen );

    // copy samples back to Java heap:
    (*env)->SetShortArrayRegion( env, outBuf, 0, ainfo->cinfo.round_samples, jsamples );

    aacd_decode_info2java( env, &ainfo->cinfo, ainfo->aacInfo );

    ainfo->env = NULL;

    return (jint) ainfo->cinfo.round_samples;
}


/*
 * Class:     com_spoledge_aacplayer_ArrayDecoder
 * Method:    nativeStop
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_spoledge_aacplayer_ArrayDecoder_nativeStop
  (JNIEnv *env, jobject thiz, jint jinfo)
{
    AACDArrayInfo *ainfo = (AACDArrayInfo*) jinfo;
    ainfo->env = env;
    aacda_stop( ainfo );
}

