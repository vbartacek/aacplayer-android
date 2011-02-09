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

import java.nio.ByteBuffer;
import java.nio.ShortBuffer;


public final class FAADDecoder extends Decoder {

    private static final String LOG = "FAADDecoder";

    private static enum State { IDLE, RUNNING };

    private static boolean libLoaded = false;


    ////////////////////////////////////////////////////////////////////////////
    // Attributes
    ////////////////////////////////////////////////////////////////////////////

    private int aacdw;
    private State state = State.IDLE;


    ////////////////////////////////////////////////////////////////////////////
    // Constructors
    ////////////////////////////////////////////////////////////////////////////

    private FAADDecoder() {
    }


    ////////////////////////////////////////////////////////////////////////////
    // Public
    ////////////////////////////////////////////////////////////////////////////

    /**
     * Creates a new decoder.
     */
    public static synchronized FAADDecoder create() {
        if (!libLoaded) {
            System.loadLibrary( "AACDecoder" );

            libLoaded = true;
        }

        return new FAADDecoder();
    }


    /**
     * Starts decoding AAC stream.
     */
    public Info start( ByteBuffer inputBuffer ) {
        if (state != State.IDLE) throw new IllegalStateException();
        
        Info ret = new Info();

        Log.d( LOG, "decode() pos=" + inputBuffer.position() + ", len=" + inputBuffer.remaining()
            + ", b1=" + inputBuffer.get(inputBuffer.position()) 
            + ", b2=" + inputBuffer.get(inputBuffer.position()+1));

        aacdw = nativeStart( inputBuffer, inputBuffer.position(), inputBuffer.remaining(), ret );

        state = State.RUNNING;

        return ret;
    }


    /**
     * Decodes AAC stream.
     * @return the number of samples produced (totally all channels = the length of the filled array)
     */
    public int decode( ByteBuffer inputBuffer, ByteBuffer outputBuffer ) {
        if (state != State.RUNNING) throw new IllegalStateException();

        int ret = nativeDecode( aacdw, inputBuffer, inputBuffer.position(), inputBuffer.remaining(), outputBuffer, outputBuffer.capacity()/2);

        outputBuffer.limit( ret > 0 ? ret : 0 );

        return ret;
    }


    /**
     * Stops the decoder and releases all resources.
     */
    public void stop() {
        if (aacdw != 0) {
            nativeStop( aacdw );
            aacdw = 0;
        }

        state = State.IDLE;
    }


    ////////////////////////////////////////////////////////////////////////////
    // Private
    ////////////////////////////////////////////////////////////////////////////

    private native int nativeStart( ByteBuffer inputBuffer, int offset, int length, Info info );

    private native int nativeDecode( int aacdw, ByteBuffer inputBuffer, int inOff, int inLen, ByteBuffer outputBuffer, int outLen );

    private native void nativeStop( int aacdw );

}


