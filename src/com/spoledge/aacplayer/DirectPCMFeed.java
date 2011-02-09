/*
** AACPlayer - Freeware Advanced Audio (AAC) Player for Android
** Copyright (C) 2010 Spolecne s.r.o., http://www.spoledge.com
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

import java.nio.ShortBuffer;


/**
 * This is the PCM Feeder.
 * Uses java.nio.* API.
 */
public class DirectPCMFeed implements Runnable {

    private static final String LOG = "DirectPCMFeed";


    ////////////////////////////////////////////////////////////////////////////
    // Attributes
    ////////////////////////////////////////////////////////////////////////////

    private int sampleRate;
    private int channels;
    private int minBufferSizeInBytes;

    private ShortBuffer buffer;

    private boolean stopped;


    ////////////////////////////////////////////////////////////////////////////
    // Constructors
    ////////////////////////////////////////////////////////////////////////////

    public DirectPCMFeed( int sampleRate, int channels) {
        this( sampleRate, channels,
            AudioTrack.getMinBufferSize( sampleRate, channels == 1 ?
                AudioFormat.CHANNEL_CONFIGURATION_MONO :
                AudioFormat.CHANNEL_CONFIGURATION_STEREO,
                AudioFormat.ENCODING_PCM_16BIT ) * 9 / 3 );
    }


    public DirectPCMFeed( int sampleRate, int channels, int minBufferSizeInBytes ) {
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

        atrack.play();

        short[] lsamples = null;

        while (!stopped) {
            ShortBuffer lbuffer = null;

            synchronized (this) {
                while (buffer == null && !stopped) {
                    Log.d( LOG, "run() waiting...");
                    try { wait(); } catch (InterruptedException e) {}
                    Log.d( LOG, "run() awaken...");
                }

                lbuffer = buffer;
                buffer = null;
                notify();
            }

            if (stopped) break;

            int ln = lbuffer.remaining();

            if (lbuffer.hasArray()) {
                lsamples = lbuffer.array();
            }
            else {
                if (lsamples == null || lsamples.length < lbuffer.capacity()) {
                    Log.d( LOG, "ShortBuffer not backed by array, creating own one...");
                    lsamples = null;
                    lsamples = new short[ lbuffer.capacity()];
                }

                lbuffer.get( lsamples, 0, ln );
            }

            int playedTotal = 0;

            do {
                if (playedTotal != 0) {
                    Log.d( LOG, "too fast for playback, sleeping...");
                    try { Thread.sleep( 100 ); } catch (InterruptedException e) {}
                }

                Log.d( LOG, "feeding PCM...");
                int played = atrack.write( lsamples, 0, ln );
                Log.d( LOG, "PCM fed by " + ln + " and played " + played + " samples");

                if (played < 0) {
                    Log.e( LOG, "error in playback feed: " + played );
                    stopped = true;
                    break;
                }

                if (lbuffer.hasArray()) {
                    lbuffer.clear();
                }

                playedTotal += played;
            } while (playedTotal < ln);
        }

        Log.d( LOG, "run() stopped." );
    }


    public synchronized void stop() {
        stopped = true;
        notify();
    }


    public synchronized void feed( ShortBuffer buffer ) {
        Log.d( LOG, "feed() len=" + buffer.remaining());
        while (this.buffer != null && !stopped) {
            Log.d( LOG, "feed() waiting...");
            try { wait(); } catch (InterruptedException e) {}
            Log.d( LOG, "feed() awaken");
        }

        this.buffer = buffer;

        notify();
    }

}


