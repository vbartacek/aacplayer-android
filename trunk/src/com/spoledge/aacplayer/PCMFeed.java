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

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

import android.util.Log;


/**
 * This is the parent of both PCM Feeders.
 */
public abstract class PCMFeed implements Runnable {

    private static final String LOG = "PCMFeed";


    ////////////////////////////////////////////////////////////////////////////
    // Attributes
    ////////////////////////////////////////////////////////////////////////////

    protected int sampleRate;
    protected int channels;
    protected int minBufferSizeInBytes;

    protected boolean stopped;

    /**
     * The local variable in run() method set by method acquireSamples().
     */
    protected short[] lsamples;


    ////////////////////////////////////////////////////////////////////////////
    // Constructors
    ////////////////////////////////////////////////////////////////////////////

    protected PCMFeed( int sampleRate, int channels) {
        this( sampleRate, channels,
            AudioTrack.getMinBufferSize( sampleRate, channels == 1 ?
                AudioFormat.CHANNEL_CONFIGURATION_MONO :
                AudioFormat.CHANNEL_CONFIGURATION_STEREO,
                AudioFormat.ENCODING_PCM_16BIT ) * 32 );
    }


    protected PCMFeed( int sampleRate, int channels, int minBufferSizeInBytes ) {
        this.sampleRate = sampleRate;
        this.channels = channels;
        this.minBufferSizeInBytes = minBufferSizeInBytes;
    }


    ////////////////////////////////////////////////////////////////////////////
    // Public
    ////////////////////////////////////////////////////////////////////////////


    public void run() {
        Log.d( LOG, "run(): sampleRate=" + sampleRate + ", channels=" + channels + ", minBufferSizeInBytes=" + minBufferSizeInBytes);
        AudioTrack atrack = new AudioTrack(
                                AudioManager.STREAM_MUSIC,
                                sampleRate,
                                channels == 1 ?
                                    AudioFormat.CHANNEL_CONFIGURATION_MONO :
                                    AudioFormat.CHANNEL_CONFIGURATION_STEREO,
                                AudioFormat.ENCODING_PCM_16BIT,
                                minBufferSizeInBytes,
                                AudioTrack.MODE_STREAM );

        boolean first = true;

        // total samples written to AudioTrack
        int writtenTotal = 0;

        while (!stopped) {
            // fetch the samples into our "local" variable lsamples:
            int ln = acquireSamples();

            if (stopped) {
                releaseSamples();
                break;
            }

            // samples written to AudioTrack in this round:
            int writtenNow = 0;

            do {
                if (writtenNow != 0) {
                    Log.d( LOG, "too fast for playback, sleeping...");
                    try { Thread.sleep( 50 ); } catch (InterruptedException e) {}
                }

                int written = atrack.write( lsamples, writtenNow, ln );

                if (written < 0) {
                    Log.e( LOG, "error in playback feed: " + written );
                    stopped = true;
                    break;
                }

                writtenTotal += written;
                int buffered = writtenTotal - atrack.getPlaybackHeadPosition()*channels;

                Log.d( LOG, "PCM fed by " + ln + " and written " + written + " samples - buffered " + buffered);

                if (first) {
                    if (buffered*2 >= minBufferSizeInBytes) {
                        Log.d( LOG, "start of AudioTrack - buffered " + buffered + " samples");
                        atrack.play();
                        first = false;
                    }
                    else {
                        Log.d( LOG, "start buffer not filled enough - AudioTrack not started yet");
                    }
                }

                writtenNow += written;
                ln -= written;
            } while (ln > 0);

            releaseSamples();
        }

        if (!first) atrack.stop();
        atrack.release();

        Log.d( LOG, "run() stopped." );
    }


    public synchronized void stop() {
        stopped = true;
        notify();
    }


    ////////////////////////////////////////////////////////////////////////////
    // Protected
    ////////////////////////////////////////////////////////////////////////////

    /**
     * Acquires samples into variable lsamples.
     * @return the actual size (in shorts) of the lsamples
     */
    protected abstract int acquireSamples();


    /**
     * Releases the lsamples variable.
     * This method is called always after processing the acquired lsamples.
     */
    protected abstract void releaseSamples();

}
