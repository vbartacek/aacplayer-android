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

#ifndef AAC_COMMON_H
#define AAC_COMMON_H

#include <jni.h>
#include <android/log.h>

#ifndef AACD_MODULE
#error "Please specify AACD_MODULE at the top of your file."
#endif


#ifdef AACD_LOGLEVEL_TRACE
#define AACD_TRACE(...) \
    __android_log_print(ANDROID_LOG_VERBOSE, AACD_MODULE, __VA_ARGS__)
#else
#define AACD_TRACE(...) //
#endif

#ifdef AACD_LOGLEVEL_DEBUG
#define AACD_DEBUG(...) \
    __android_log_print(ANDROID_LOG_DEBUG, AACD_MODULE, __VA_ARGS__)
#else
#define AACD_DEBUG(...) //
#endif

#ifdef AACD_LOGLEVEL_INFO
#define AACD_INFO(...) \
    __android_log_print(ANDROID_LOG_INFO, AACD_MODULE, __VA_ARGS__)
#else
#define AACD_INFO(...) //
#endif

#ifdef AACD_LOGLEVEL_WARN
#define AACD_WARN(...) \
    __android_log_print(ANDROID_LOG_WARN, AACD_MODULE, __VA_ARGS__)
#else
#define AACD_WARN(...) //
#endif

#ifdef AACD_LOGLEVEL_ERROR
#define AACD_ERROR(...) \
    __android_log_print(ANDROID_LOG_ERROR, AACD_MODULE, __VA_ARGS__)
#else
#error "Ha AACD_LOGLEVEL_ERROR is not defined"
#define AACD_ERROR(...) //
#endif


#ifdef __cplusplus
extern "C" {
#endif

/**
 * This should be returned by the decoder's decode() method.
 */
enum AACDDecodeResult {
    AACD_DECODE_OK = 0x0000,
    AACD_DECODE_EOF = 0x0001,
    AACD_DECODE_INPUT_NEEDED = 0x0100,
    AACD_DECODE_OUTPUT_NEEDED = 0x0200,
    AACD_DECODE_OTHER = 0x8000
};


/**
 * Common info struct used for storing info between calls.
 */
typedef struct AACDCommonInfo {
    // flag for controlling the input - decoder reads the input data itself:
    int input_ctrl;

    unsigned long samplerate;
    unsigned char channels;
    unsigned long bytesconsumed;
    unsigned long bytesleft;

    unsigned char *buffer;
    unsigned long bbsize;

    // decode() function will fill these:
    unsigned long frame_bytesconsumed;
    unsigned long frame_samples;

    // max statistics allowing to predict when to finish decoding:
    unsigned long frame_max_bytesconsumed;
    unsigned long frame_max_bytesconsumed_exact;
    unsigned long frame_max_samples;

    // filled after each decoding round
    unsigned long round_frames;
    unsigned long round_bytesconsumed;
    unsigned long round_samples;

} AACDCommonInfo;


/**
 * Decoder definition.
 */
typedef struct AACDDecoder {
    /**
     * Returns the name of the decoder.
     */
    const char* (*name)();

    /**
     * Initializes the decoder.
     * @param ext the decoder can allocate private data and modify the pointer
     * @return 0 on success; otherwise an error code
     */
    int (*init)(void** pext);

    /**
     * Start decoding.
     * Must fill at least sampleRate and number of channels (AACDCommonInfo).
     * @return either positive = number of bytes consumed; negative means an error code.
     */
    long (*start)( AACDCommonInfo*, void*, unsigned char*, unsigned long);

    /**
     * Decodes one frame.
     * @return AACDDecodeResult (0=OK)
     */
    int (*decode)( AACDCommonInfo*, void*, unsigned char *, unsigned long, jshort*, jint);

    /**
     * Destroys the decoder - the decoder should free all resources.
     */
    void (*destroy)( AACDCommonInfo*, void*);

    /**
     * Synchronizes stream.
     * @return the number of bytes to be skipped or -1 if no sync word found.
     */
    int (*sync)( unsigned char *buffer, int len );
} AACDDecoder;


/**************************************************************************************************
 * Functions
 *************************************************************************************************/

/**
 * Searches for ADTS 0xfff header.
 * Returns the offset of ADTS frame.
 */
int aacd_probe(unsigned char *buffer, int len);


/**
 * Copies relevant information to Java object.
 * This is called in the start method.
 */
void aacd_start_info2java( JNIEnv *env, AACDCommonInfo *info, jobject jinfo );


/**
 * Copies relevant information to Java object.
 * This is called in the decode method.
 */
void aacd_decode_info2java( JNIEnv *env, AACDCommonInfo *cinfo, jobject jinfo );


#ifdef __cplusplus
}
#endif
#endif
