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

#include "aac-array-common.h"

#include <string.h>
#include <android/log.h>

#define AACDW "ArrayDecoder[Common]"

struct JavaArrayBufferReader {
    jclass bufferClazz;
    jfieldID bufferData;
    jfieldID bufferSize;
    jclass clazz;
    jmethodID next;
};

static struct JavaArrayBufferReader javaABR;


/****************************************************************************************************
 * FUNCTIONS
 ****************************************************************************************************/

AACDArrayInfo* aacda_start( JNIEnv *env, AACDDecoder *decoder, jobject jreader, jobject aacInfo)
{
    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "start() starting native service - %s", decoder->name());

    AACDArrayInfo *ainfo = (AACDArrayInfo*) calloc( 1, sizeof( struct AACDArrayInfo ));

    ainfo->decoder = decoder;

    ainfo->ext = ainfo->decoder->init();

    ainfo->reader = (*env)->NewGlobalRef( env, jreader );
    ainfo->aacInfo = (*env)->NewGlobalRef( env, aacInfo );

    return ainfo;
}


void aacda_stop( AACDArrayInfo *ainfo )
{
    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "stop() stopping native service" );

    if (ainfo == NULL) return;

    AACDCommonInfo *cinfo = &ainfo->cinfo;

    if (ainfo->decoder) ainfo->decoder->destroy( cinfo, ainfo->ext );

    if (ainfo->buffer_block != NULL)
    {
        free( ainfo->buffer_block );
        ainfo->buffer_block = NULL;
        cinfo->bbsize = 0;
    }

    if (ainfo->buffer_block2 != NULL)
    {
        free( ainfo->buffer_block2 );
        ainfo->buffer_block = NULL;
        ainfo->bbsize2 = 0;
    }

    if (ainfo->samples != NULL)
    {
        free( ainfo->samples );
        ainfo->samplesLen = 0;
    }

    JNIEnv *env = ainfo->env;

    if (ainfo->aacInfo) (*env)->DeleteGlobalRef( env, ainfo->aacInfo );
    if (ainfo->reader) (*env)->DeleteGlobalRef( env, ainfo->reader );

    free( ainfo );
}


unsigned char* aacda_prepare_buffer( AACDArrayInfo *ainfo, jbyteArray inBuf, jint inOff, jint inLen )
{
    AACDCommonInfo *cinfo = &ainfo->cinfo;

    int newlen = cinfo->bytesleft + inLen;

    if (ainfo->bbsize2 < newlen) 
    {
        if (ainfo->buffer_block2 != NULL) free( ainfo->buffer_block2 );

        int realsize = newlen + 500; // avoid realocating by one or two bytes only

        ainfo->buffer_block2 = (unsigned char*) malloc( realsize );
        ainfo->bbsize2 = realsize;
    }

    if (cinfo->bytesleft != 0) memcpy( ainfo->buffer_block2, cinfo->buffer, cinfo->bytesleft );

    JNIEnv *env = ainfo->env;
    (*env)->GetByteArrayRegion( env, inBuf, inOff, inLen, ainfo->buffer_block2 + cinfo->bytesleft );

    // memcpy( ainfo->buffer_block2 + cinfo->bytesleft, jbuffer + inOff, inLen );

    cinfo->buffer = ainfo->buffer_block;
    ainfo->buffer_block = ainfo->buffer_block2;
    ainfo->buffer_block2 = cinfo->buffer;
    cinfo->buffer = ainfo->buffer_block;

    int tmp;
    tmp = cinfo->bbsize;
    cinfo->bbsize = ainfo->bbsize2;
    ainfo->bbsize2 = tmp;

    cinfo->bytesleft += inLen;

    return cinfo->buffer;
}


unsigned char* aacda_read_buffer( AACDArrayInfo *ainfo )
{
    JNIEnv *env = ainfo->env;

    if (javaABR.clazz == NULL)
    {
        javaABR.clazz = (*env)->GetObjectClass( env, ainfo->reader );
        javaABR.next = (*env)->GetMethodID( env, javaABR.clazz, "next", "()Lcom/spoledge/aacplayer/ArrayBufferReader$Buffer;");

        javaABR.bufferClazz = (*env)->FindClass( env, "com/spoledge/aacplayer/ArrayBufferReader$Buffer");
        javaABR.bufferData = (jfieldID) (*env)->GetFieldID( env, javaABR.bufferClazz, "data", "[B");
        javaABR.bufferSize = (jfieldID) (*env)->GetFieldID( env, javaABR.bufferClazz, "size", "I");
    }

    jobject jbuffer = (*env)->CallObjectMethod( env, ainfo->reader, javaABR.next );
    
    if (!jbuffer) return NULL;

    jbyteArray data = (jbyteArray) (*env)->GetObjectField( env, jbuffer, javaABR.bufferData );
    jint size = (*env)->GetIntField( env, jbuffer, javaABR.bufferSize );

    return aacda_prepare_buffer( ainfo, data, 0, size );
}


jshort* aacda_prepare_samples( AACDArrayInfo *ainfo, jint outLen )
{
    if (ainfo->samplesLen < outLen)
    {
        if (ainfo->samples) free( ainfo->samples );
        ainfo->samples = malloc( sizeof( jshort ) * outLen );
        ainfo->samplesLen = outLen;
    }

    return ainfo->samples;
}


void aacda_decode( AACDArrayInfo *ainfo, jshort *samples, jint outLen )
{
    AACDCommonInfo *cinfo = &ainfo->cinfo;

    cinfo->round_frames = 0;
    cinfo->round_bytesconsumed = 0;
    cinfo->round_samples = 0;

    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "decode() start");

    do
    {
        // check if input buffer is filled:
        if (cinfo->bytesleft <= cinfo->frame_max_bytesconsumed)
        {
            aacda_read_buffer( ainfo );

            if (cinfo->bytesleft <= cinfo->frame_max_bytesconsumed)
            {
                __android_log_print(ANDROID_LOG_DEBUG, AACDW, "decode() detected eof");
                break;
            }
        }

        int attempts = 10;

        do
        {
            if (!ainfo->decoder->decode( cinfo, ainfo->ext, cinfo->buffer, cinfo->bytesleft, samples, outLen )) break;

            __android_log_print(ANDROID_LOG_INFO, AACDW, "decode() frame - frames=%d, consumed=%d, samples=%d, bytesleft=%d, frame_maxconsumed=%d, frame_samples=%d, outLen=%d", cinfo->round_frames, cinfo->round_bytesconsumed, cinfo->round_samples, cinfo->bytesleft, cinfo->frame_max_bytesconsumed, cinfo->frame_samples, outLen);

            if (cinfo->bytesleft <= cinfo->frame_max_bytesconsumed)
            {
                aacda_read_buffer( ainfo );

                if (cinfo->bytesleft <= cinfo->frame_max_bytesconsumed)
                {
                    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "decode() no more input - eof");
                    attempts = 0;
                    break;
                }
            }

            int pos = aacd_probe( cinfo->buffer+1, cinfo->bytesleft-1 );
            cinfo->buffer += pos+1;
            cinfo->bytesleft -= pos+1;
        }
        while (--attempts > 0);

        if ( !attempts )
        {
            __android_log_print(ANDROID_LOG_WARN, AACDW, "decode() failed");
            break;
        }

        cinfo->round_frames++;
        cinfo->round_bytesconsumed += cinfo->frame_bytesconsumed;
        cinfo->bytesleft -= cinfo->frame_bytesconsumed;
        cinfo->buffer += cinfo->frame_bytesconsumed;

        if (cinfo->frame_bytesconsumed > cinfo->frame_max_bytesconsumed)
        {
            cinfo->frame_max_bytesconsumed_exact = cinfo->frame_bytesconsumed;
            cinfo->frame_max_bytesconsumed = cinfo->frame_bytesconsumed * 3 / 2;
        }

        samples += cinfo->frame_samples;
        outLen -= cinfo->frame_samples;
        cinfo->round_samples += cinfo->frame_samples;
    } 
    while (outLen >= cinfo->frame_samples );

    __android_log_print(ANDROID_LOG_DEBUG, AACDW, "decode() round - frames=%d, consumed=%d, samples=%d, bytesleft=%d, frame_maxconsumed=%d, frame_samples=%d, outLen=%d", cinfo->round_frames, cinfo->round_bytesconsumed, cinfo->round_samples, cinfo->bytesleft, cinfo->frame_max_bytesconsumed, cinfo->frame_samples, outLen);
}

