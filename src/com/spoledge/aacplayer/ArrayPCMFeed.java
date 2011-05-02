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

import android.util.Log;

import java.nio.ShortBuffer;


/**
 * This is the PCM Feeder which uses arrays (short[]).
 */
public class ArrayPCMFeed extends PCMFeed {

    private static final String LOG = "ArrayPCMFeed";


    ////////////////////////////////////////////////////////////////////////////
    // Attributes
    ////////////////////////////////////////////////////////////////////////////

    private int sampleRate;
    private int channels;
    private int minBufferSizeInBytes;

    private short[] samples;
    private int n;

    private boolean stopped;


    ////////////////////////////////////////////////////////////////////////////
    // Constructors
    ////////////////////////////////////////////////////////////////////////////

    /**
     * Creates a new threaded PCM feeder which uses arrays.
     * @param sampleRate the sampling rate
     * @param channels the number of channels (0 or 1)
     */
    public ArrayPCMFeed( int sampleRate, int channels) {
        super( sampleRate, channels );
    }


    /**
     * Creates a new threaded PCM feeder which uses arrays.
     * @param sampleRate the sampling rate
     * @param channels the number of channels (0 or 1)
     * @param minBufferSizeInBytes the size of buffer used by AudioTrack object
     */
    public ArrayPCMFeed( int sampleRate, int channels, int minBufferSizeInBytes ) {
        super( sampleRate, channels, minBufferSizeInBytes );
    }


    ////////////////////////////////////////////////////////////////////////////
    // Public
    ////////////////////////////////////////////////////////////////////////////

    /**
     * This is called by main thread when a new data are available.
     */
    public synchronized void feed( short[] samples, int n ) {
        while (this.samples != null && !stopped) {
            try { wait(); } catch (InterruptedException e) {}
        }

        this.samples = samples;
        this.n = n;

        notify();
    }


    ////////////////////////////////////////////////////////////////////////////
    // Protected
    ////////////////////////////////////////////////////////////////////////////

    /**
     * Acquires samples into variable lsamples.
     * @return the actual size (in shorts) of the lsamples
     */
    protected synchronized int acquireSamples() {
        while (n == 0 && !stopped) {
            try { wait(); } catch (InterruptedException e) {}
        }

        lsamples = samples;
        int ln = n;

        samples = null;
        n = 0;
        notify();

        return ln;
    }


    /**
     * Releases the lsamples variable.
     * This method is called always after processing the acquired lsamples.
     */
    protected void releaseSamples() {
        // nothing to be done
    }

}

