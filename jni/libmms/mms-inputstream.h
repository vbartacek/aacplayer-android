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

/* Header for class com_spoledge_aacplayer_MMSInputStream */

#ifndef _Included_com_spoledge_aacplayer_MMSInputStream
#define _Included_com_spoledge_aacplayer_MMSInputStream

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Class:     com_spoledge_aacplayer_MMSInputStream
 * Method:    nativeConnect
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_spoledge_aacplayer_MMSInputStream_nativeConnect
  (JNIEnv *, jobject, jstring);

/*
 * Class:     com_spoledge_aacplayer_MMSInputStream
 * Method:    nativeRead
 * Signature: (I[BII)I
 */
JNIEXPORT jint JNICALL Java_com_spoledge_aacplayer_MMSInputStream_nativeRead
  (JNIEnv *, jobject, jint, jbyteArray, jint, jint);

/*
 * Class:     com_spoledge_aacplayer_MMSInputStream
 * Method:    nativeClose
 * Signature: (I)I
 */
JNIEXPORT void JNICALL Java_com_spoledge_aacplayer_MMSInputStream_nativeClose
  (JNIEnv *, jobject, jint);

#ifdef __cplusplus
}
#endif
#endif
