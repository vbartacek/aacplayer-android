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

#define ANDROID_LOG_MODULE "MMSInputStream[native]"

#include "config.h"
#include "mms-inputstream.h"
#include "android-log.h"
#include "src/mmsx.h"

#include <string.h>

typedef struct MMSInfo {
    unsigned char* buffer;
    unsigned long bbsize;
    mmsx_t *mms;
} MMSInfo;


/*
 * Class:     com_spoledge_aacplayer_MMSInputStream
 * Method:    nativeConnect
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_spoledge_aacplayer_MMSInputStream_nativeConnect
  (JNIEnv *env, jobject thiz, jstring jurl)
{
    ALOG_TRACE( "nativeConnect() start" );

    jsize url_clen = (*env)->GetStringLength( env, jurl );
    jsize url_blen = (*env)->GetStringUTFLength( env, jurl );

    char *url = calloc( 1, url_blen + 1 );

    (*env)->GetStringUTFRegion( env, jurl, 0, url_clen, url );

    ALOG_DEBUG( "nativeConnect() url='%s'", url );

    // now create mms_io for passing JNIEnv to it:
    mms_io_t *mms_io = calloc( 1, sizeof( mms_io_t ));
    mms_io->android_jni = env;

    mmsx_t *mms = mmsx_connect( NULL, NULL, url, 128000 );

    free( mms_io );
    free( url );

    if (!mms)
    {
        ALOG_DEBUG( "nativeConnect() return 0" );
        return 0;
    }

    MMSInfo *minfo = (MMSInfo*) calloc( 1, sizeof(struct MMSInfo));
    minfo->mms = mms;

    ALOG_DEBUG( "nativeConnect() info=%p", minfo );
    ALOG_TRACE( "nativeConnect() end" );

    return (jint) minfo;
}


/*
 * Class:     com_spoledge_aacplayer_MMSInputStream
 * Method:    nativeRead
 * Signature: (I[BII)I
 */
JNIEXPORT jint JNICALL Java_com_spoledge_aacplayer_MMSInputStream_nativeRead
  (JNIEnv *env, jobject thiz, jint jminfo, jbyteArray jbuf, jint off, jint len)
{
    ALOG_TRACE( "nativeRead() start" );

    MMSInfo *minfo = (MMSInfo*) jminfo;

    ALOG_DEBUG( "nativeRead() info=%p", minfo );

    if (len > minfo->bbsize)
    {
        ALOG_TRACE( "nativeRead() extending buffer %d -> %d bytes", minfo->bbsize, len );
        if (minfo->buffer) free( minfo->buffer );

        minfo->buffer = malloc( len );
        minfo->bbsize = len;
    }

    ALOG_TRACE( "nativeRead() calling mmsx_read len=%d", len );

    int n = mmsx_read( NULL, minfo->mms, minfo->buffer, len );

    ALOG_TRACE( "nativeRead() got %d bytes from mmsx_read", n );

    (*env)->SetByteArrayRegion( env, jbuf, off, n, minfo->buffer );

    ALOG_TRACE( "nativeRead() end" );

    return n;
}


/*
 * Class:     com_spoledge_aacplayer_MMSInputStream
 * Method:    nativeClose
 * Signature: (I)I
 */
JNIEXPORT void JNICALL Java_com_spoledge_aacplayer_MMSInputStream_nativeClose
  (JNIEnv *env, jobject thiz, jint jminfo)
{
    ALOG_TRACE( "nativeClose() start" );

    MMSInfo *minfo = (MMSInfo*) jminfo;

    if (!minfo) return;

    mmsx_close( minfo->mms );

    if (minfo->buffer) free( minfo->buffer );
    free( minfo );

    ALOG_TRACE( "nativeClose() end" );
}


int android_string_utf16(void *android_jni, char *dest, char *src, int dest_len)
{
    ALOG_TRACE( "android_string_utf16() start" );

    JNIEnv *env = (JNIEnv*) android_jni;
    int iblen = strlen(src);
    int oblen = dest_len - 2; /* reserve 2 bytes for 0 termination */

    jstring jsrc = (*env)->NewStringUTF( env, src );
    int clen = (*env)->GetStringLength( env, jsrc );

    int ret = oblen - clen * 2;
    if (ret < 0)
    {
        clen = oblen / 2;
        ret = 0;
    }

    (*env)->GetStringRegion( env, jsrc, 0, clen, (jchar*) dest);

    jchar *oc = (jchar*)(dest + clen*2);
    *oc = 0;

    ALOG_TRACE( "android_string_utf16() end" );

    return ret;
}

