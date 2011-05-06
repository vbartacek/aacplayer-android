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

#define AACD_MODULE "Decoder[FAAD2]"

#include "aac-array-common.h"
#include "neaacdec.h"


static const char* aacd_faad_name()
{
    return "FAAD2";
}


static void* aacd_faad_init()
{
    void *ret = NeAACDecOpen();
    AACD_INFO( "init() FAAD2 capabilities: %d", NeAACDecGetCapabilities());

    return ret;
}


static void aacd_faad_destroy( AACDCommonInfo *cinfo, void *ext )
{
    if ( ext ) NeAACDecClose( ext );
}


static long aacd_faad_start( AACDCommonInfo *cinfo, void *ext, unsigned char *buffer, unsigned long buffer_size)
{
    NeAACDecConfigurationPtr conf = NeAACDecGetCurrentConfiguration( ext );
    conf->outputFormat = FAAD_FMT_16BIT;
    conf->downMatrix = 1;
    //conf->useOldADTSFormat = 1;

    NeAACDecSetConfiguration( ext, conf);

    AACD_DEBUG( "start() buffer=%d size=%d", (*(unsigned long*)buffer), buffer_size );

    long ret = NeAACDecInit( ext, buffer, buffer_size, &cinfo->samplerate, &cinfo->channels );

    if (ret < 0) AACD_ERROR( "NeAACDecInit failed err=%d", ret );

    return ret;
}


static int aacd_faad_decode( AACDCommonInfo *cinfo, void *ext, unsigned char *buffer, unsigned long buffer_size, jshort *jsamples, jint outLen )
{
    NeAACDecFrameInfo frame;
    jshort *ljsamples = jsamples;

    NeAACDecDecode2( ext, &frame, buffer, buffer_size, (void**)&jsamples, outLen*2 );

    if (ljsamples != jsamples) {
        AACD_WARN( "NeAACDecDecode CHANGE jsamples !!!");
    }

    cinfo->frame_bytesconsumed = frame.bytesconsumed;
    cinfo->frame_samples = frame.samples;

    if (frame.error != 0)
    {
        AACD_ERROR( "NeAACDecDecode bytesleft=%d, error: %s",
                    buffer_size,
                    NeAACDecGetErrorMessage(frame.error));
    }

    return frame.error;
}


AACDDecoder aacd_faad_decoder = {
    aacd_faad_name,
    aacd_faad_init,
    aacd_faad_start,
    aacd_faad_decode,
    aacd_faad_destroy
};
