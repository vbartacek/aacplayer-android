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

import java.nio.ShortBuffer;


/**
 * This is the PCM Feeder.
 * Uses java.nio.* API.
 */
public class DirectPCMFeed extends AbstractPCMFeed {

    private static final String LOG = "DirectPCMFeed";


    ////////////////////////////////////////////////////////////////////////////
    // Attributes
    ////////////////////////////////////////////////////////////////////////////

    /**
     * This variable holds buffer to be processed.
     */
    private ShortBuffer buffer;

    /**
     * This variable holds local buffer being processed by run() method.
     */
    private ShortBuffer lbuffer;


    ////////////////////////////////////////////////////////////////////////////
    // Constructors
    ////////////////////////////////////////////////////////////////////////////

    /**
     * Creates a new threaded PCM feeder which uses java.nio.* buffers.
     * @param sampleRate the sampling rate
     * @param channels the number of channels (0 or 1)
     */
    public DirectPCMFeed( int sampleRate, int channels) {
        super( sampleRate, channels );
    }


    /**
     * Creates a new threaded PCM feeder which uses java.nio.* buffers.
     * @param sampleRate the sampling rate
     * @param channels the number of channels (0 or 1)
     * @param minBufferSizeInBytes the size of buffer used by AudioTrack object
     */
    public DirectPCMFeed( int sampleRate, int channels, int minBufferSizeInBytes ) {
        super( sampleRate, channels, minBufferSizeInBytes );
    }


    ////////////////////////////////////////////////////////////////////////////
    // Public
    ////////////////////////////////////////////////////////////////////////////

    /**
     * This is called by main thread when a new data are available.
     */
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


    ////////////////////////////////////////////////////////////////////////////
    // Protected
    ////////////////////////////////////////////////////////////////////////////

    /**
     * Acquires samples into variable lsamples.
     * @return the actual size (in shorts) of the lsamples
     */
    protected int acquireSamples() {
        synchronized (this) {
            while (buffer == null && !stopped) {
                try { wait(); } catch (InterruptedException e) {}
            }

            lbuffer = buffer;
            buffer = null;
            notify();
        }

        if (stopped) return 0;

        int ln = lbuffer.remaining();

        if (lbuffer.hasArray()) {
            lsamples = lbuffer.array();
        }
        else {
            if (lsamples == null || lsamples.length < lbuffer.capacity()) {
                Log.i( LOG, "ShortBuffer not backed by array, creating own one...");
                lsamples = null;
                lsamples = new short[ lbuffer.capacity()];
            }

// hmm, this seems bad:
long ts = System.currentTimeMillis();
            lbuffer.get( lsamples, 0, ln );
Log.d( LOG, "PCM lbuffer.get() " + (System.currentTimeMillis()-ts) + " ms");
        }

        return ln;
    }


    /**
     * Releases the lsamples variable.
     * This method is called always after processing the acquired lsamples.
     */
    protected void releaseSamples() {
        if (lbuffer != null && lbuffer.hasArray()) {
            lbuffer.clear();
        }
    }

}

