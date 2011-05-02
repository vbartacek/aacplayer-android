/*
** AACPlayer - Freeware Advanced Audio (AAC) Player for Android
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
package com.spoledge.aacplayer;

import android.util.Log;

import java.io.FileInputStream;
import java.io.InputStream;
import java.io.IOException;

import java.net.URL;
import java.net.URLConnection;


/**
 * This is the AACPlayer which uses AACDecoder to decode AAC stream into PCM samples.
 * Uses java.nio.* API.
 */
public class ArrayAACPlayer extends AACPlayer {

    private static final String LOG = "ArrayAACPlayer";

    private ArrayDecoder decoder;


    ////////////////////////////////////////////////////////////////////////////
    // Constructors
    ////////////////////////////////////////////////////////////////////////////

    public ArrayAACPlayer( ArrayDecoder decoder ) {
        this( decoder, null );
    }


    public ArrayAACPlayer( ArrayDecoder decoder, PlayerCallback playerCallback ) {
        super( playerCallback );
        this.decoder = decoder;
    }


    ////////////////////////////////////////////////////////////////////////////
    // Protected
    ////////////////////////////////////////////////////////////////////////////

    protected void playImpl( InputStream is, int expectedKBitSecRate ) throws Exception {
        ArrayBufferReader reader = new ArrayBufferReader( computeInputBufferSize( expectedKBitSecRate, roundDurationMs ), is );
        new Thread( reader ).start();

        ArrayPCMFeed pcmfeed = null;

        // profiling info
        long profMs = 0;
        long profSamples = 0;
        long profSampleRate = 0;
        int profCount = 0;

        try {
            Decoder.Info info = decoder.start( reader );

            Log.d( LOG, "play(): samplerate=" + info.getSampleRate() + ", channels=" + info.getChannels());

            profSampleRate = info.getSampleRate() * info.getChannels();

            if (info.getChannels() > 2) {
                throw new RuntimeException("Too many channels detected: " + info.getChannels());
            }

            // 3 buffers for result samples:
            //   - one is used by decoder
            //   - one is used by the PCMFeeder
            //   - one is enqueued / passed to PCMFeeder - non-blocking op
            int samplesCapacity = computeOutputBufferSize( info.getSampleRate(), info.getChannels(), roundDurationMs);

            Log.d( LOG, "run() output capacity (samples)=" + samplesCapacity );

            short[][] buffers = new short[3][];

            for (int i=0; i < buffers.length; i++) {
                buffers[i] = new short[ samplesCapacity ];
            }

            short[] outputBuffer = buffers[0]; 

            int samplespoolindex = 0;

            pcmfeed = new ArrayPCMFeed( info.getSampleRate(), info.getChannels());
            new Thread(pcmfeed).start();

            do {
                long tsStart = System.currentTimeMillis();

                info = decoder.decode( outputBuffer, samplesCapacity );
                int nsamp = info.getRoundSamples();

                profMs += System.currentTimeMillis() - tsStart;
                profSamples += nsamp;
                profCount++;

                Log.d( LOG, "play(): decoded " + nsamp + " samples" );

                if (nsamp == 0) stopped = true;

                if (stopped) break;

                pcmfeed.feed( outputBuffer, nsamp );
                if (stopped) break;

                // we subtract 1 to avoid samples buffer overrun:
                int kBitSecRate = computeAvgKBitSecRate( info ) - 1;
                if (Math.abs(expectedKBitSecRate - kBitSecRate) > 1) {
                    Log.d( LOG, "play(): changing kBitSecRate: " + expectedKBitSecRate + " -> " + kBitSecRate );
                    reader.setCapacity( computeInputBufferSize( kBitSecRate, roundDurationMs ));
                    expectedKBitSecRate = kBitSecRate;
                }

                outputBuffer = buffers[ ++samplespoolindex % 3 ];

                Log.d( LOG, "play(): yield, sleeping...");
                try { Thread.sleep( 50 ); } catch (InterruptedException e) {}
            } while (!stopped);
        }
        finally {
            stopped = true;

            if (pcmfeed != null) pcmfeed.stop();
            decoder.stop();

            if (profCount > 0) Log.i( LOG, "play(): average decoding time: " + profMs / profCount + " ms");
            if (profMs > 0) Log.i( LOG, "play(): average rate (samples/sec): audio=" + profSampleRate
                + ", decoding=" + (1000*profSamples / profMs)
                + ", audio/decoding= " + (1000*profSamples / profMs - profSampleRate) * 100 / profSampleRate + " %  (the higher, the better; negative means that decoding is slower than needed by audio)");

            if (playerCallback != null) playerCallback.playerStopped();
        }
    }

}
