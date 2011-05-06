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


        ////////////////////////////////////////////////////////////////////////////
        // Public
        ////////////////////////////////////////////////////////////////////////////

        /**
         * Returns the sampling rate in Hz.
         * @return the sampling rate - always set
         */
        public int getSampleRate() {
            return sampleRate;
        }


        /**
         * Returns the number of channels (0=unknown yet, 1=mono, 2=stereo).
         * @return the channels - always set
         */
        public int getChannels() {
            return channels;
        }


        /**
         * Returns the maximum bytes consumed per ADTS frame.
         * @return the value - after each decode() round
         */
        public int getFrameMaxBytesConsumed() {
            return frameMaxBytesConsumed;
        }


        /**
         * Returns the samples produced per ADTS frame.
         * @return the value - after each decode() round (but should be always the same)
         */
        public int getFrameSamples() {
            return frameSamples;
        }


        /**
         * Returns the number of ADTS frames decoded.
         * @return the value - after each decode() round
         */
        public int getRoundFrames() {
            return roundFrames;
        }


        /**
         * Returns the number of bytes consumed.
         * @return the value - after each decode() round
         */
        public int getRoundBytesConsumed() {
            return roundBytesConsumed;
        }


        /**
         * Returns the number of samples decoded.
         * @return the value - after each decode() round
         */
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

