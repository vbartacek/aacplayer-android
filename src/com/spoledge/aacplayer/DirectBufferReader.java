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

import java.io.IOException;

import java.nio.ByteBuffer;
import java.nio.channels.ReadableByteChannel;


/**
 * This is a separate thread for reading data from a channel.
 * Uses java.nio.* API.
 */
public class DirectBufferReader implements Runnable {
    private static String LOG = "DirectBufferReader";

    int capacity;
    int minSize;
    int minOffset;
    private ReadableByteChannel rbc;
    private ByteBuffer[] buffers;

    /**
     * The buffer to be write into.
     */
    private int indexMine;

    /**
     * The index of the buffer last returned in the next() method.
     */
    private int indexBlocked;

    private boolean stopped;


    ////////////////////////////////////////////////////////////////////////////
    // Constructors
    ////////////////////////////////////////////////////////////////////////////

    /**
     * Creates a new buffer.
     * @param capacity the capacity of one buffer
     * @param minSize the min count of read bytes
     * @param minOffset the min offset - can be used to prepend some bytes later
     * @param rbc a readable channel that should be created in blocking mode.
     */
    public DirectBufferReader( int capacity, int minSize, int minOffset, ReadableByteChannel rbc ) {
        if (capacity < minSize) throw new RuntimeException("Capacity cannot be less than minSize");

        this.capacity = capacity;
        this.minSize = minSize;
        this.minOffset = minOffset;
        this.rbc = rbc;

        buffers = new ByteBuffer[3];

        for (int i=0; i < buffers.length; i++) {
            buffers[i] = ByteBuffer.allocateDirect( capacity );
        }
    }


    ////////////////////////////////////////////////////////////////////////////
    // Public
    ////////////////////////////////////////////////////////////////////////////


    /**
     * The main loop.
     */
    public void run() {
        Log.d( LOG, "run() started...." );

        indexMine = 0;
        indexBlocked = buffers.length-1;

        int total = 0;
        int min = minSize;

        while (!stopped) {
            ByteBuffer buffer = buffers[ indexMine ];
            buffer.clear();
            buffer.position( minOffset );
            total = 0;

            while (!stopped && total < min) {
                try {
                    int n = rbc.read( buffer );

                    if (n == -1) stopped = true;
                    else total += n;
                    //Log.d( LOG, "run() read " + total + " bytes" );
                }
                catch (IOException e) {
                    Log.e( LOG, "Exception when reading: " + e );
                    stopped = true;
                }

                // When consumer is fast, then read only minimum required bytes.
                if ((indexBlocked + 1) % buffers.length == indexMine) min = minSize;
            }

            buffer.flip();
            buffer.position( minOffset );
//            Log.d( LOG, "run() buffer[" + indexMine + "] = " + buffer.remaining() + "  bytes" );

            synchronized (this) {
                notify();
                int indexNew = (indexMine + 1) % buffers.length;

                while (!stopped && indexNew == indexBlocked) {
                    Log.d( LOG, "run() waiting...." );
                    try { wait(); } catch (InterruptedException e) {}
                    Log.d( LOG, "run() awaken" );
                }

                indexMine = indexNew;
                min = buffer.capacity() - minOffset;
            }
        }

        Log.d( LOG, "run() stopped." );
    }


    /**
     * Stops the thread - the object cannot be longer used.
     */
    public synchronized void stop() {
        stopped = true;
        notify();
    }


    /**
     * Returns true if this thread was stopped.
     */
    public boolean isStopped() {
        return stopped;
    }


    /**
     * Returns next available direct buffer instance.
     * The returned instance can be freely used by another thread.
     * Blocks the caller until a buffer is ready.
     */
    public synchronized ByteBuffer next() {
//        Log.d( LOG, "next() indexMine=" + indexMine + ", indexBlocked=" + indexBlocked);

        int indexNew = (indexBlocked + 1) % buffers.length;

        while (!stopped && indexNew == indexMine) {
            Log.d( LOG, "next() waiting...." );
            try { wait(); } catch (InterruptedException e) {}
            Log.d( LOG, "next() awaken" );
        }

        if (indexNew == indexMine) return null;

        indexBlocked = indexNew;

//        Log.d( LOG, "next() buffer[" + indexBlocked + "] = " + buffers[ indexBlocked ].remaining() + "  bytes" );

        notify();

        return buffers[ indexBlocked ]; 
    }


}
