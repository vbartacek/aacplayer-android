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

#include <jni.h>
/* Header for class com_spoledge_aacplayer_Decoder */

#ifndef _Included_com_spoledge_aacplayer_Decoder
#define _Included_com_spoledge_aacplayer_Decoder

#ifdef __cplusplus
extern "C" {
#endif

#undef com_spoledge_aacplayer_Decoder_DECODER_FAAD2
#define com_spoledge_aacplayer_Decoder_DECODER_FAAD2 1L

#undef com_spoledge_aacplayer_Decoder_DECODER_FFMPEG
#define com_spoledge_aacplayer_Decoder_DECODER_FFMPEG 2L

#undef com_spoledge_aacplayer_Decoder_DECODER_OPENCORE
#define com_spoledge_aacplayer_Decoder_DECODER_OPENCORE 4L

#ifdef __cplusplus
}
#endif
#endif
