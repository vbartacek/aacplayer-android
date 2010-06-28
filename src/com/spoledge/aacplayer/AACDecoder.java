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

public final class AACDecoder {
    private static final String LOG = "AACDecoder";

    public static class AACInfo {
        private int samplerate;
        private int channels;

        public int getChannels() {
            return channels;
        }

        public int getSamplerate() {
            return samplerate;
        }
    }

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

    private AACDecoder() {
    }


    ////////////////////////////////////////////////////////////////////////////
    // Public
    ////////////////////////////////////////////////////////////////////////////

    /**
     * Creates a new decoder.
     */
    public static synchronized AACDecoder create() {
        if (!libLoaded) {
            System.loadLibrary( "AACDecoder" );

            libLoaded = true;
        }

        return new AACDecoder();
    }


    /**
     * Starts decoding AAC stream.
     */
    public AACInfo start( byte[] buf, int off, int len ) {
        if (state != State.IDLE) throw new IllegalStateException();
        
        AACInfo ret = new AACInfo();

        aacdw = nativeStart( buf, off, len, ret );

        state = State.RUNNING;

        return ret;
    }


    /**
     * Decodes AAC stream.
     * @return the number of samples produced (totally all channels = the length of the filled array)
     */
    public int decode( byte[] buf, int off, int len, short[] samples, int outLen ) {
        if (state != State.RUNNING) throw new IllegalStateException();

        return nativeDecode( aacdw, buf, off, len, samples, outLen );
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
    // Protected
    ////////////////////////////////////////////////////////////////////////////

    @Override
    protected void finalize() {
        try {
            stop();
        }
        catch (Throwable t) {
            t.printStackTrace();
        }
    }


    ////////////////////////////////////////////////////////////////////////////
    // Private
    ////////////////////////////////////////////////////////////////////////////

    private native int nativeStart( byte[] buf, int off, int len, AACInfo aacInfo );

    private native int nativeDecode( int aacdw, byte[] buf, int off, int len, short[] samples, int outLen );

    private native void nativeStop( int aacdw );

}
