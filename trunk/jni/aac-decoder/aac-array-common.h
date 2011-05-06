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

#ifndef AAC_ARRAY_COMMON_H
#define AAC_ARRAY_COMMON_H

#include "aac-common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AACDArrayInfo {
    AACDCommonInfo cinfo;

    // internal input buffer
    unsigned char *buffer_block;
    unsigned char *buffer_block2;
    unsigned long bbsize2;

    // internal output buffer
    jshort *samples;
    unsigned long samplesLen;

    //
    // The following fields to be moved to AACDCommonInfo:
    //

    // the last known JNIEnv:
    JNIEnv *env;

    // the input buffer reader object:
    jobject *reader;

    // the callback variable - Decoder.Info:
    jobject *aacInfo;

    // the decoders impl methods:
    AACDDecoder *decoder;

    // extended info - each decoder can use it for its own purposes:
    void *ext;

} AACDArrayInfo;



/**
 * Starts the service - initializes resource.
 */
AACDArrayInfo* aacda_start( JNIEnv *env, AACDDecoder *decoder, jobject jreader, jobject aacInfo);


/**
 * Stops the service and frees resources.
 */
void aacda_stop( AACDArrayInfo *ainfo );


/**
 * Prepares input buffer by joining the rest of the old one and the new one.
 */
unsigned char* aacda_prepare_buffer( AACDArrayInfo *ainfo, jbyteArray inBuf, jint inOff, jint inLen );


/**
 * Reads next buffer.
 */
unsigned char* aacda_read_buffer( AACDArrayInfo *ainfo );


/**
 * Prepares output buffer.
 */
jshort* aacda_prepare_samples( AACDArrayInfo *ainfo, jint outLen );


/**
 * Decodes the stream - one round until the output buffer is (almost) filled.
 */
void aacda_decode( AACDArrayInfo *ainfo, jshort *samples, jint outLen );


#ifdef __cplusplus
}
#endif
#endif

