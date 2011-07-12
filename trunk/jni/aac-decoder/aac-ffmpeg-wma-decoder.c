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

#define AACD_MODULE "Decoder[FFMPEG/WMA]"

#include "aac-array-common.h"
#include <string.h>

#include "libavcodec/avcodec.h"
#include "libavcodec/aac_parser.h"
#include "libavcodec/get_bits.h"
#include "libavcodec/mpeg4audio.h"
#include "libavutil/mem.h"
#include "libavutil/log.h"

#include "libavformat/avformat.h"


typedef struct AACDFFmpegInfo {
    AACDCommonInfo *cinfo;
    AVInputFormat *avifmt;
    AVFormatContext *avfctx;
    AVPacket *avpkt;
    AVPacket *pkt;
    int audio_stream_index;

    unsigned int bytesconsumed;
} AACDFFmpegInfo;


extern AVCodec wmav1_decoder;
extern AVCodec wmav2_decoder;
extern AVInputFormat asf_demuxer;


static const char* aacd_ffwma_name()
{
    return "FFmpeg/WMA";
}


static const char *aacd_ffwma_logname( void *ctx )
{
    return AACD_MODULE;
}


/**
 * Creates a new AVPacket.
 */
static AVPacket* aacd_ff_create_avpkt()
{
    AVPacket *avpkt = (AVPacket*) av_mallocz( sizeof(AVPacket));
    avpkt->data = NULL;
    avpkt->size = 0;

    return avpkt;
}


/**
 * A wrapper method which reads packets.
 * It only reads them from the internal pre-fetched buffer in AACDCommonInfo.
 */
static int aacd_ff_io_read_packet( void *opaque, uint8_t *buf, int buf_size)
{
    AACD_TRACE( "io_read_packet() start" );

    AACDFFmpegInfo *ff = (AACDFFmpegInfo*) opaque;
    AACDCommonInfo *cinfo = ff->cinfo;

    if (cinfo->bytesleft < buf_size)
    {
        // Let's cheat now:
        AACDArrayInfo *ainfo = (AACDArrayInfo*) cinfo;
        if (!aacda_read_buffer( ainfo ))
        {
            AACD_INFO( "io_read_packet() EOF detected" );
        }
    }

    int len = buf_size < cinfo->bytesleft ? buf_size : cinfo->bytesleft;

    if (!len)
    {
        AACD_WARN( "read_packet(): no bytes left, returning 0" );
        return 0;
    }

    memcpy( buf, cinfo->buffer, len );

    cinfo->buffer += len;
    cinfo->bytesleft -= len;

    ff->bytesconsumed += len;

    AACD_TRACE( "io_read_packet() stop" );

    return len;
}


/**
 * Creates a new ByteIOContext.
 */
static ByteIOContext* aacd_ff_create_byteioctx( AACDFFmpegInfo *ff )
{
    int buffer_size = ff->cinfo->bbsize;
    unsigned char *buffer = av_mallocz( buffer_size );
    ByteIOContext *pb = av_alloc_put_byte( buffer, buffer_size, 0, ff, aacd_ff_io_read_packet, NULL, NULL);

    if (!pb)
    {
        av_free( buffer );
        AACD_WARN( "create_byteioctx(): ByteIOContext could not be created" );
    }

    return pb;
}


/**
 * Destroys a ByteIOContext.
 */
static void aacd_ff_destroy_byteioctx( ByteIOContext *pb )
{
    if (!pb) return;
    if (pb->buffer) av_free( pb->buffer );
    av_free( pb );
}


/**
 * Destroys our context.
 */
static void aacd_ff_destroy( AACDFFmpegInfo *ff )
{
    if ( !ff ) return;

    AACD_TRACE( "destroy() start" );

    AVFormatContext *ic = ff->avfctx;

    if ( ic )
    {
        if ( ff->audio_stream_index > -1) avcodec_close( ic->streams[ff->audio_stream_index]->codec );

        ByteIOContext *pb = ic->pb;

        av_close_input_stream( ff->avfctx );

        if ( pb ) aacd_ff_destroy_byteioctx( pb );
    }

    if (ff->avpkt) av_free( ff->avpkt );
    if (ff->pkt) av_free( ff->pkt );

    av_free( ff );

    AACD_TRACE( "destroy() stop" );
}


static void aacd_ffwma_stop( AACDCommonInfo *cinfo, void *ext )
{
    if ( !ext ) return;

    AACDFFmpegInfo *ff = (AACDFFmpegInfo*) ext;

    aacd_ff_destroy( ff );
}


/**
 * Initializes our context.
 */
static int aacd_ff_init( void **pext, AVInputFormat *fmt  )
{
    AACD_TRACE( "init() start" );

    av_log_set_level( AV_LOG_DEBUG );

    AACDFFmpegInfo *ff = (AACDFFmpegInfo*) av_mallocz( sizeof(struct AACDFFmpegInfo));

    if (!ff) return -1;

    ff->avpkt = aacd_ff_create_avpkt();
    ff->pkt = aacd_ff_create_avpkt();

    if (!ff->avpkt || !ff->pkt)
    {
        AACD_ERROR( "init() out of memory error !" );
        aacd_ff_destroy( ff );
        return -2;
    }

    ff->avifmt = fmt;
    ff->audio_stream_index = -1;

    av_log( ff->avfctx, AV_LOG_INFO, "Test of AV_LOG_INFO\n" );
    av_log( ff->avfctx, AV_LOG_DEBUG, "Test of AV_LOG_DEBUG\n" );
    av_log( ff->avfctx, AV_LOG_VERBOSE, "Test of AV_LOG_VERBOSE\n" );

    (*pext) = ff;

    AACD_TRACE( "init() stop" );

    return 0;
}


static int aacd_ffwma_init( void **pext )
{
    return aacd_ff_init( pext, &asf_demuxer );
}



/**
 * Finds a stream's position or return -1.
 * This method also discards all streams.
 */
static int aacd_ff_find_stream( AVFormatContext *ic, enum AVMediaType codec_type )
{
    int i;
    int ret = -1;

    for (i = 0; i < ic->nb_streams; i++)
    {
        AVStream *st = ic->streams[i];
        AVCodecContext *avctx = st->codec;

        st->discard = AVDISCARD_ALL;

        if (ret == -1 && avctx->codec_type == codec_type) ret = i;
    }

    return ret;
}


/**
 * Simple method for looking up only our supported codecs.
 */
static AVCodec* aacd_ffwma_find_codec( enum CodecID id )
{
   switch (id)
   {
       case CODEC_ID_WMAV1: return &wmav1_decoder;
       case CODEC_ID_WMAV2: return &wmav2_decoder;
   }

   return NULL;
}


static long aacd_ffwma_start( AACDCommonInfo *cinfo, void *ext, unsigned char *buffer, unsigned long buffer_size)
{
    AACD_TRACE( "start() start" );

    AACDFFmpegInfo *ff = (AACDFFmpegInfo*) ext;
    ff->cinfo = cinfo;

    // take control over the input reading:
    cinfo->input_ctrl = 1;

    ByteIOContext *pb = aacd_ff_create_byteioctx( ff );
    if (!pb) return -1;

    AACD_TRACE( "start() opening stream" );
    ff->bytesconsumed = 0;

    int err = av_open_input_stream( &ff->avfctx, pb, "filename.asf", ff->avifmt, NULL );
    AVFormatContext *ic = ff->avfctx;

    if (err)
    {
        char s[80];
        av_strerror( err, s, 80);
        AACD_ERROR("start() cannot open demuxer - [%d] - $s", err, s );

        // we must dealloc what we allocated locally:
        aacd_ff_destroy_byteioctx( pb );

        return -1;
    }

    AACD_TRACE( "start() stream opened" );
    //err = av_find_stream_info(ic)
    AACD_DEBUG( "start() streams=%d", ic->nb_streams);
    dump_format(ic, 0, "", 0);

    ff->audio_stream_index = aacd_ff_find_stream( ic, AVMEDIA_TYPE_AUDIO );

    if (ff->audio_stream_index < 0) 
    {
        AACD_ERROR( "start() cannot find audio stream" );

        return -1;
    }

    AVStream *st = ic->streams[ff->audio_stream_index];
    st->discard = AVDISCARD_DEFAULT;

    AVCodecContext *avctx = st->codec;
    AACD_DEBUG( "start() samplerate=%d channels=%d codec=%x", avctx->sample_rate, avctx->channels, avctx->codec_id);

    AVCodec *codec = aacd_ffwma_find_codec( avctx->codec_id );

    if (!codec)
    {
        AACD_ERROR("start() audio - not a WMA codec - %x", avctx->codec_id);

        return -1;
    }

    if (avcodec_open( avctx, codec ))
    {
        AACD_ERROR("start() audio cannot open audio codec - %x", avctx->codec_id);

        return -1;
    }

    cinfo->samplerate = avctx->sample_rate;
    cinfo->channels = avctx->channels;

    AACD_TRACE( "start() stop - ic->format=%x, ff->avfctx->format=%x", ic->iformat, ff->avfctx->iformat );

    // we return more than we consumed:
    return ff->bytesconsumed;
}


static int aacd_ffwma_decode( AACDCommonInfo *cinfo, void *ext, unsigned char *buffer, unsigned long buffer_size, jshort *jsamples, jint outLen )
{
    AACD_TRACE( "decode() start" );

    AACDFFmpegInfo *ff = (AACDFFmpegInfo*) ext;
    AVFormatContext *ic = ff->avfctx;
    AVPacket *avpkt = ff->avpkt;
    AVPacket *pkt = ff->pkt;

    ff->bytesconsumed = 0;

#ifdef AACD_LOGLEVEL_TRACE
    ic->debug = FF_FDEBUG_TS;
#endif

    while (!pkt->size)
    {
        AACD_TRACE( "decode() calling av_read_frame..." );
        int err = av_read_frame( ic, avpkt );
        AACD_TRACE( "decode() av_read_frame returned: %d", err );

        if (err < 0)
        {
            AACD_ERROR( "decode() cannot read av frame" );

            return AACD_DECODE_EOF;
        }

        if (avpkt->stream_index == ff->audio_stream_index)
        {
            pkt->data = avpkt->data;
            pkt->size = avpkt->size;
            break;
        }

        // TODO: delete packet's buffer ?
        AACD_TRACE( "decode() : freeing packet's data" );
        av_freep( &avpkt->data );
    }

    AACD_TRACE( "decode() packet demuxed, will decode..." );

    AVCodecContext *avctx = ic->streams[ff->audio_stream_index]->codec;
    AVCodec *codec = avctx->codec;

    AACD_DEBUG( "decode() frame_size=%d", avctx->frame_size );

    // aac_decode_frame
    int outSize = outLen * 2;
    int consumed = (*codec->decode)( avctx, jsamples, &outSize, pkt );

    if (consumed <= 0)
    {
        AACD_ERROR( "decode() cannot decode frame pkt->size=%d, outSize=%d, error: %d", pkt->size, outSize, consumed );

        if ( cinfo->frame_samples < outLen * 3 / 2 )
        {
            AACD_WARN( "decode() trying to obtain large output buffer" );

            return AACD_DECODE_OUTPUT_NEEDED;
        }

        pkt->size = 0;

        return AACD_DECODE_OTHER;
    }

    pkt->data += consumed;
    pkt->size -= consumed;

    cinfo->frame_bytesconsumed = consumed;
    //cinfo->frame_samples = avctx->frame_size * avctx->channels;
    cinfo->frame_samples = (outSize >> 1);

    AACD_TRACE( "decode() stop - consumed %d, pkt->size=%d", consumed, pkt->size );

    return AACD_DECODE_OK;
}


static int aacd_ffwma_probe( unsigned char *buffer, int len )
{
    return 0;
}



AACDDecoder aacd_ffmpeg_wma_decoder = {
    aacd_ffwma_name,
    aacd_ffwma_init,
    aacd_ffwma_start,
    aacd_ffwma_decode,
    aacd_ffwma_stop,
    aacd_ffwma_probe
};


