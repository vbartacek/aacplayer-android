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

#define AACD_MODULE "Decoder[FFMPEG]"

#include "aac-array-common.h"
#include <string.h>

#include "libavcodec/avcodec.h"
#include "libavcodec/aac_parser.h"
#include "libavcodec/get_bits.h"
#include "libavcodec/mpeg4audio.h"
#include "libavutil/mem.h"
#include "libavutil/log.h"


typedef struct AACDFFmpeg {
    AVCodecContext *avctx;
    AVPacket *avpkt;
} AACDFFmpeg;


extern AVCodec aac_decoder;


static const char* aacd_ffmpeg_name()
{
    return "FFmpeg";
}


static const char *aacd_ffmpeg_logname( void *ctx )
{
    return AACD_MODULE;
}


/**
 * Creates a new AVCodecContext.
 */
static AVCodecContext* aacd_ffmpeg_create_avctx( AVCodec *codec ) 
{
    AVCodecContext *avctx = (AVCodecContext*) av_mallocz( sizeof(AVCodecContext));

    AVClass *avcls = (AVClass*) av_mallocz( sizeof(AVClass));
    avcls->class_name = "AVCodecContext";
    avcls->item_name = aacd_ffmpeg_logname;

    avctx->av_class = avcls;
    avctx->codec = codec;
    memcpy( avctx->codec_name, codec->name, strlen( codec->name ));
    avctx->codec_type = codec->type;
    avctx->codec_id = codec->id;
    avctx->priv_data = (void*) av_mallocz( codec->priv_data_size );
    avctx->extradata_size = 0;

    return avctx;
}


/**
 * Creates a new AVPacket.
 */
static AVPacket* aacd_ffmpeg_create_avpkt()
{
    AVPacket *avpkt = (AVPacket*) av_mallocz( sizeof(AVPacket));
    avpkt->data = NULL;
    avpkt->size = 0;

    return avpkt;
}


static void* aacd_ffmpeg_init()
{
    av_log_set_level( AV_LOG_DEBUG );

    AACDFFmpeg *ff = (AACDFFmpeg*) av_mallocz( sizeof(struct AACDFFmpeg));

    ff->avctx = aacd_ffmpeg_create_avctx( &aac_decoder );
    ff->avpkt = aacd_ffmpeg_create_avpkt();

    av_log( ff->avctx, AV_LOG_INFO, "Test of AV_LOG_INFO\n" );
    av_log( ff->avctx, AV_LOG_DEBUG, "Test of AV_LOG_DEBUG\n" );
    av_log( ff->avctx, AV_LOG_VERBOSE, "Test of AV_LOG_VERBOSE\n" );

    return ff;
}


static void aacd_ffmpeg_destroy( AACDCommonInfo *cinfo, void *ext )
{
    if ( !ext ) return;

    AACDFFmpeg *ff = (AACDFFmpeg*) ext;
    AVCodecContext *avctx = ff->avctx;

    if ( avctx )
    {
        if (avctx->codec) avctx->codec->close( avctx );
        if (avctx->av_class) av_free( (void*) avctx->av_class );
        av_free( avctx );
    }

    if (ff->avpkt) av_free( ff->avpkt );

    av_free( ff );
}


static long aacd_ffmpeg_start( AACDCommonInfo *cinfo, void *ext, unsigned char *buffer, unsigned long buffer_size)
{
    AACDFFmpeg *ff = (AACDFFmpeg*) ext;
    AVCodecContext *avctx = ff->avctx;
    AVCodec *codec = avctx->codec;

    codec->init( avctx );

    // Decode one frame to obtain sample rate and channels
    AVPacket *avpkt = ff->avpkt;
    avpkt->data = buffer;
    avpkt->size = buffer_size;
    avpkt->pos = 0;

    int outSize = 4096 * sizeof(jshort);
    jshort* tmp = av_mallocz(outSize);

    int consumed = codec->decode( avctx, tmp, &outSize, avpkt );
    av_free( tmp );

    if (consumed <= 0) {
        AACD_ERROR("start() cannot decode first frame, error: %d", consumed );

        return -1;
    }

    cinfo->samplerate = avctx->sample_rate;
    cinfo->channels = avctx->channels;

    return consumed;
}


static int aacd_ffmpeg_decode( AACDCommonInfo *cinfo, void *ext, unsigned char *buffer, unsigned long buffer_size, jshort *jsamples, jint outLen )
{
    AACDFFmpeg *ff = (AACDFFmpeg*) ext;
    AVCodecContext *avctx = ff->avctx;
    AVPacket *avpkt = ff->avpkt;
    AVCodec *codec = avctx->codec;

    avpkt->data = buffer;
    avpkt->size = buffer_size;
    avpkt->pos = 0;

    // aac_decode_frame
    int outSize = outLen * 2;
    int consumed = (*codec->decode)( avctx, jsamples, &outSize, avpkt );

    if (consumed <= 0) {
        AACD_ERROR( "decode() cannot decode frame bytesleft=%d, error: %d", buffer_size, consumed );

        return -1;
    }

    cinfo->frame_bytesconsumed = consumed;
    cinfo->frame_samples = avctx->frame_size * avctx->channels;

    return 0;
}


AACDDecoder aacd_ffmpeg_decoder = {
    aacd_ffmpeg_name,
    aacd_ffmpeg_init,
    aacd_ffmpeg_start,
    aacd_ffmpeg_decode,
    aacd_ffmpeg_destroy
};

