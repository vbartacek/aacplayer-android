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
package com.spoledge.aacplayer;

import android.util.Log;

import java.io.FileInputStream;
import java.io.InputStream;
import java.io.IOException;

import java.net.URL;
import java.net.URLConnection;


/**
 * This is the AACPlayer which uses ArrayDecoder to decode AAC stream into PCM samples.
 * <pre>
 *  ArrayDecoder decoder = ArrayDecoder.create( Decoder.DECODER_OPENCORE );
 *  ArrayAACPlayer player = new ArrayAACPlayer( decoder );
 *
 *  String url = ...;
 *  player.playAsync( url );
 * </pre>
 */
public class ArrayAACPlayer extends AACPlayer {

    private static final String LOG = "ArrayAACPlayer";

    private ArrayDecoder decoder;


    ////////////////////////////////////////////////////////////////////////////
    // Constructors
    ////////////////////////////////////////////////////////////////////////////

    /**
     * Creates a new player.
     * @param decoder the underlying decoder
     */
    public ArrayAACPlayer( ArrayDecoder decoder ) {
        this( decoder, null );
    }


    /**
     * Creates a new player.
     * @param decoder the underlying decoder
     * @param playerCallback the callback, can be null
     */
    public ArrayAACPlayer( ArrayDecoder decoder, PlayerCallback playerCallback ) {
        this( decoder, playerCallback, DEFAULT_AUDIO_BUFFER_CAPACITY_MS, DEFAULT_DECODE_BUFFER_CAPACITY_MS );
    }


    /**
     * Creates a new player.
     * @param decoder the underlying decoder
     * @param playerCallback the callback, can be null
     * @param audioBufferCapacityMs the capacity of the audio buffer (AudioTrack) in ms
     * @param decodeBufferCapacityMs the capacity of the buffer used for decoding in ms
     * @see setAudioBufferCapacityMs(int)
     * @see setDecodeBufferCapacityMs(int)
     */
    public ArrayAACPlayer( ArrayDecoder decoder, PlayerCallback playerCallback,
                            int audioBufferCapacityMs, int decodeBufferCapacityMs ) {

        super( playerCallback, audioBufferCapacityMs, decodeBufferCapacityMs );

        this.decoder = decoder;
    }


    ////////////////////////////////////////////////////////////////////////////
    // Protected
    ////////////////////////////////////////////////////////////////////////////

    /**
     * Plays a stream synchronously.
     * This is the implementation method calle by every play() and playAsync() methods.
     * @param is the input stream
     * @param expectedKBitSecRate the expected average bitrate in kbit/sec
     */
    protected void playImpl( InputStream is, int expectedKBitSecRate ) throws Exception {
        ArrayBufferReader reader = new ArrayBufferReader(
                                        computeInputBufferSize( expectedKBitSecRate, decodeBufferCapacityMs ),
                                        is );
        new Thread( reader ).start();

        ArrayPCMFeed pcmfeed = null;
        Thread pcmfeedThread = null;

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
            short[][] decodeBuffers = createDecodeBuffers( 3, info );
            short[] decodeBuffer = decodeBuffers[0]; 
            int decodeBufferIndex = 0;

            pcmfeed = createArrayPCMFeed( info );
            pcmfeedThread = new Thread( pcmfeed );
            pcmfeedThread.start();

            do {
                long tsStart = System.currentTimeMillis();

                info = decoder.decode( decodeBuffer, decodeBuffer.length );
                int nsamp = info.getRoundSamples();

                profMs += System.currentTimeMillis() - tsStart;
                profSamples += nsamp;
                profCount++;

                Log.d( LOG, "play(): decoded " + nsamp + " samples" );

                if (nsamp == 0 || stopped) break;
                if (!pcmfeed.feed( decodeBuffer, nsamp ) || stopped) break;

                int kBitSecRate = computeAvgKBitSecRate( info );
                if (Math.abs(expectedKBitSecRate - kBitSecRate) > 1) {
                    Log.i( LOG, "play(): changing kBitSecRate: " + expectedKBitSecRate + " -> " + kBitSecRate );
                    reader.setCapacity( computeInputBufferSize( kBitSecRate, decodeBufferCapacityMs ));
                    expectedKBitSecRate = kBitSecRate;
                }

                decodeBuffer = decodeBuffers[ ++decodeBufferIndex % 3 ];
            } while (!stopped);
        }
        finally {
            stopped = true;

            if (pcmfeed != null) pcmfeed.stop();
            decoder.stop();
            reader.stop();

            int perf = 0;

            if (profCount > 0) Log.i( LOG, "play(): average decoding time: " + profMs / profCount + " ms");

            if (profMs > 0) {
                perf = (int)((1000*profSamples / profMs - profSampleRate) * 100 / profSampleRate);

                Log.i( LOG, "play(): average rate (samples/sec): audio=" + profSampleRate
                    + ", decoding=" + (1000*profSamples / profMs)
                    + ", audio/decoding= " + perf
                    + " %  (the higher, the better; negative means that decoding is slower than needed by audio)");
            }

            if (pcmfeedThread != null) pcmfeedThread.join();

            if (playerCallback != null) playerCallback.playerStopped( perf );
        }
    }


    ////////////////////////////////////////////////////////////////////////////
    // Private
    ////////////////////////////////////////////////////////////////////////////

    private short[][] createDecodeBuffers( int count, Decoder.Info info ) {
        int size = PCMFeed.msToSamples( decodeBufferCapacityMs, info.getSampleRate(), info.getChannels());

        short[][] ret = new short[ count ][];

        for (int i=0; i < ret.length; i++) {
            ret[i] = new short[ size ];
        }

        return ret;
    }


    private ArrayPCMFeed createArrayPCMFeed( Decoder.Info info ) {
        int size = PCMFeed.msToBytes( audioBufferCapacityMs, info.getSampleRate(), info.getChannels());

        return new ArrayPCMFeed( info.getSampleRate(), info.getChannels(), size, playerCallback );
    }

}
