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

#define AACD_MODULE "CommonDecoder"

#include "aac-common.h"

#include <string.h>

struct JavaDecoderInfo {
    jclass clazz;
    jfieldID sampleRate;
    jfieldID channels;
    jfieldID frameMaxBytesConsumed;
    jfieldID frameSamples;
    jfieldID roundFrames;
    jfieldID roundBytesConsumed;
    jfieldID roundSamples;
};

static struct JavaDecoderInfo javaDecoderInfo;


int aacd_probe(unsigned char *buffer, int len)
{
    int pos = 0;
    len -= 3;

    AACD_TRACE( "probe() start len=%d", len );

    while (pos < len)
    {
        if (*buffer != 0xff)
        {
            buffer++;
            pos++;
        }
        else if ((*(++buffer) & 0xf6) == 0xf0)
        {
            AACD_TRACE( "probe() found ADTS start at offset %d", pos );
            return pos;
        }
        else pos++;
    }

    AACD_WARN( "probe() could not find ADTS start" );

    return 0;
}


void aacd_start_info2java( JNIEnv *env, AACDCommonInfo *cinfo, jobject jinfo )
{
    if (javaDecoderInfo.clazz == NULL)
    {
        AACD_TRACE( "aacd_start_info2java() - caching Java reflection" );
        javaDecoderInfo.clazz = (jclass) (*env)->GetObjectClass( env, jinfo );
        javaDecoderInfo.sampleRate = (jfieldID) (*env)->GetFieldID( env, javaDecoderInfo.clazz, "sampleRate", "I");
        javaDecoderInfo.channels = (jfieldID) (*env)->GetFieldID( env, javaDecoderInfo.clazz, "channels", "I");
        javaDecoderInfo.frameMaxBytesConsumed = (jfieldID) (*env)->GetFieldID( env, javaDecoderInfo.clazz, "frameMaxBytesConsumed", "I");
        javaDecoderInfo.frameSamples = (jfieldID) (*env)->GetFieldID( env, javaDecoderInfo.clazz, "frameSamples", "I");
        javaDecoderInfo.roundFrames = (jfieldID) (*env)->GetFieldID( env, javaDecoderInfo.clazz, "roundFrames", "I");
        javaDecoderInfo.roundBytesConsumed = (jfieldID) (*env)->GetFieldID( env, javaDecoderInfo.clazz, "roundBytesConsumed", "I");
        javaDecoderInfo.roundSamples = (jfieldID) (*env)->GetFieldID( env, javaDecoderInfo.clazz, "roundSamples", "I");
    }

    AACD_TRACE( "aacd_start_info2java() - storing info sampleRate=%d, channels=%d",
            cinfo->samplerate, cinfo->channels );

    (*env)->SetIntField( env, jinfo, javaDecoderInfo.sampleRate, (jint) cinfo->samplerate);
    (*env)->SetIntField( env, jinfo, javaDecoderInfo.channels, (jint) cinfo->channels);

    AACD_TRACE( "aacd_start_info2java() - finished" );
}


void aacd_decode_info2java( JNIEnv *env, AACDCommonInfo *cinfo, jobject jinfo )
{
    AACD_TRACE( "aacd_decode_info2java() - storing info frameMaxBytesConsumed=%d, frameSamples=%d, roundFrames=%d, roundBytesConsumed=%d, roundSamples=%d",
            cinfo->frame_max_bytesconsumed, cinfo->frame_samples,
            cinfo->round_frames, cinfo->round_bytesconsumed, cinfo->round_samples );

    (*env)->SetIntField( env, jinfo, javaDecoderInfo.frameMaxBytesConsumed, (jint) cinfo->frame_max_bytesconsumed);
    (*env)->SetIntField( env, jinfo, javaDecoderInfo.frameSamples, (jint) cinfo->frame_samples);
    (*env)->SetIntField( env, jinfo, javaDecoderInfo.roundFrames, (jint) cinfo->round_frames);
    (*env)->SetIntField( env, jinfo, javaDecoderInfo.roundBytesConsumed, (jint) cinfo->round_bytesconsumed);
    (*env)->SetIntField( env, jinfo, javaDecoderInfo.roundSamples, (jint) cinfo->round_samples);

    AACD_TRACE( "aacd_decode_info2java() - finished" );
}


