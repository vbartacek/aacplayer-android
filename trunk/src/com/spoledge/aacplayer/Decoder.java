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

import java.nio.ByteBuffer;
import java.nio.ShortBuffer;


/**
 * Parent class for all decoders.
 */
public abstract class Decoder {

    /**
     * Info about the stream.
     */
    public static class Info {
        private int sampleRate;
        private int channels;

        public int getChannels() {
            return channels;
        }

        public int getSampleRate() {
            return sampleRate;
        }
    }


    ////////////////////////////////////////////////////////////////////////////
    // Public
    ////////////////////////////////////////////////////////////////////////////


    /**
     * Starts decoding stream.
     */
    public abstract Info start( ByteBuffer inputBuffer );


    /**
     * Decodes AAC stream.
     * @return the number of samples produced (totally all channels = the length of the filled array)
     */
    public abstract int decode( ByteBuffer inputBuffer, ByteBuffer outputBuffer );


    /**
     * Stops the decoder and releases all resources.
     */
    public abstract void stop();


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

}

