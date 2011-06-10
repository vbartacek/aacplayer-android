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


/**
 * Parent class for all array decoders.
 */
public class ArrayDecoder extends Decoder {

    private static enum State { IDLE, RUNNING };

    private static boolean libLoaded = false;


    ////////////////////////////////////////////////////////////////////////////
    // Attributes
    ////////////////////////////////////////////////////////////////////////////

    private int decoder;
    private int aacdw;
    private State state = State.IDLE;


    ////////////////////////////////////////////////////////////////////////////
    // Constructors
    ////////////////////////////////////////////////////////////////////////////

    private ArrayDecoder( int decoder ) {
        this.decoder = decoder;
    }


    ////////////////////////////////////////////////////////////////////////////
    // Public
    ////////////////////////////////////////////////////////////////////////////

    /**
     * Returns supported decoders.
     * @return the supported decoders (bit array)
     * @see DECODER_FAAD2
     * @see DECODER_FFMPEG
     * @see DECODER_OPENCORE
     */
    public static int getFeatures() {
        loadLibrary();

        return nativeGetFeatures();
    }


    /**
     * Creates a new decoder.
     * @param decoder the decoder to be used
     * @see DECODER_FAAD2
     * @see DECODER_FFMPEG
     * @see DECODER_OPENCORE
     */
    public static synchronized ArrayDecoder create( int decoder ) {
        loadLibrary();

        return new ArrayDecoder( decoder );
    }


    /**
     * Returns the decoder.
     * @see DECODER_FAAD2
     * @see DECODER_FFMPEG
     * @see DECODER_OPENCORE
     */
    public int getDecoder() {
        return decoder;
    }


    /**
     * Starts decoding AAC stream.
     */
    public Info start( ArrayBufferReader reader ) {
        if (state != State.IDLE) throw new IllegalStateException();

        info = new Info();

        aacdw = nativeStart( decoder, reader, info );

        if (aacdw == 0) throw new RuntimeException("Cannot start native decoder");

        state = State.RUNNING;

        return info;
    }


    /**
     * Decodes AAC stream.
     * @return the number of samples produced (totally all channels = the length of the filled array)
     */
    public Info decode( short[] samples, int outLen ) {
        if (state != State.RUNNING) throw new IllegalStateException();

        nativeDecode( aacdw, samples, outLen );

        return info;
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

    private static synchronized void loadLibrary() {
        if (!libLoaded) {
            System.loadLibrary( "aacarray" );

            libLoaded = true;
        }
    }


    private static native int nativeGetFeatures();

    private native int nativeStart( int decoder, ArrayBufferReader reader, Info info );

    private native int nativeDecode( int aacdw, short[] samples, int outLen );

    private native void nativeStop( int aacdw );
}


