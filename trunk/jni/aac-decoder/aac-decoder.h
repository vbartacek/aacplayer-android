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

#include <jni.h>

/* Header for class com_spoledge_aacplayer_AACDecoder */

#ifndef _Included_com_spoledge_aacplayer_AACDecoder
#define _Included_com_spoledge_aacplayer_AACDecoder
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_spoledge_aacplayer_AACDecoder
 * Method:    nativeStart
 * Signature: ([BIILcom/spoledge/aacplayer/AACDecoder/AACInfo;)J
 */
JNIEXPORT jint JNICALL Java_com_spoledge_aacplayer_AACDecoder_nativeStart
  (JNIEnv *, jobject, jbyteArray, jint, jint, jobject);

/*
 * Class:     com_spoledge_aacplayer_AACDecoder
 * Method:    nativeDecode
 * Signature: (J[BII[B)I
 */
JNIEXPORT jint JNICALL Java_com_spoledge_aacplayer_AACDecoder_nativeDecode
  (JNIEnv *, jobject, jint, jbyteArray, jint, jint, jshortArray, jint);

/*
 * Class:     com_spoledge_aacplayer_AACDecoder
 * Method:    nativeStop
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_com_spoledge_aacplayer_AACDecoder_nativeStop
  (JNIEnv *, jobject, jint);

#ifdef __cplusplus
}
#endif
#endif
