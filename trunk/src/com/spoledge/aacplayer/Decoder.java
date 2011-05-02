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


/**
 * Parent class for all decoders.
 */
public abstract class Decoder {

    /**
     * Info about the stream.
     */
    public static final class Info {
        private int sampleRate;
        private int channels;

        private int frameMaxBytesConsumed;
        private int frameSamples;

        private int roundFrames;
        private int roundBytesConsumed;
        private int roundSamples;

        public int getChannels() {
            return channels;
        }

        public int getSampleRate() {
            return sampleRate;
        }

        public int getFrameMaxBytesConsumed() {
            return frameMaxBytesConsumed;
        }

        public int getFrameSamples() {
            return frameSamples;
        }

        public int getRoundFrames() {
            return roundFrames;
        }

        public int getRoundBytesConsumed() {
            return roundBytesConsumed;
        }

        public int getRoundSamples() {
            return roundSamples;
        }
    
    }


    /**
     * Decoder supported bit: FAAD2.
     */
    public static final int DECODER_FAAD2 = 0x01;

    /**
     * Decoder supported bit: FFMPEG.
     */
    public static final int DECODER_FFMPEG = 0x02;

    /**
     * Decoder supported bit: OpenCORE.
     */
    public static final int DECODER_OPENCORE = 0x04;


    private static boolean libLoaded = false;


    ////////////////////////////////////////////////////////////////////////////
    // Attributes
    ////////////////////////////////////////////////////////////////////////////

    /**
     * The information passed between JNI and Java.
     */
    protected Info info;

}

