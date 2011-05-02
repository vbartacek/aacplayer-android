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

import java.io.FileInputStream;
import java.io.InputStream;
import java.io.IOException;

import java.net.URL;
import java.net.URLConnection;


/**
 * This is the AACPlayer parent class.
 * It uses Decoder to decode AAC stream into PCM samples.
 * This class is not thread safe.
 */
public abstract class AACPlayer {

    public static final int DEFAULT_EXPECTED_KBITSEC_RATE = 64;

    private static final String LOG = "AACPlayer";


    ////////////////////////////////////////////////////////////////////////////
    // Attributes
    ////////////////////////////////////////////////////////////////////////////

    protected boolean stopped;
    protected PlayerCallback playerCallback;

    protected int roundDurationMs = 750;

    private int sumKBitSecRate = 0;
    private int countKBitSecRate = 0;


    ////////////////////////////////////////////////////////////////////////////
    // Constructors
    ////////////////////////////////////////////////////////////////////////////

    public AACPlayer() {
    }


    public AACPlayer( PlayerCallback playerCallback ) {
        setPlayerCallback( playerCallback );
    }


    ////////////////////////////////////////////////////////////////////////////
    // Public
    ////////////////////////////////////////////////////////////////////////////

    public void setPlayerCallback( PlayerCallback playerCallback ) {
        this.playerCallback = playerCallback;
    }

    public PlayerCallback getPlayerCallback() {
        return playerCallback;
    }


    public void stop() {
        stopped = true;
    }


    public void playAsync( final String url ) {
        playAsync( url, -1 );
    }


    public void playAsync( final String url, final int expectedKBitSecRate ) {
        new Thread(new Runnable() {
            public void run() {
                try {
                    play( url, expectedKBitSecRate );
                }
                catch (Exception e) {
                    Log.e( LOG, "playAsync():", e);

                    if (playerCallback != null) playerCallback.playerException( e );
                }
            }
        }).start();
    }


    public void play( String url ) throws Exception {
        play( url, -1 );
    }


    public void play( String url, int expectedKBitSecRate ) throws Exception {
        if (url.indexOf( ':' ) > 0) {
            URLConnection cn = new URL( url ).openConnection();
            cn.connect();

            dumpHeaders( cn );

            // TODO: try to get the expectedKBitSecRate from headers
            play( cn.getInputStream(), expectedKBitSecRate);
        }
        else play( new FileInputStream( url ), expectedKBitSecRate );
    }


    public void play( InputStream is ) throws Exception {
        play( is, -1 );
    }


    public final void play( InputStream is, int expectedKBitSecRate ) throws Exception {
        stopped = false;

        if (playerCallback != null) playerCallback.playerStarted();

        if (expectedKBitSecRate <= 0) expectedKBitSecRate = DEFAULT_EXPECTED_KBITSEC_RATE;

        sumKBitSecRate = 0;
        countKBitSecRate = 0;

        playImpl( is, expectedKBitSecRate );
    }


    ////////////////////////////////////////////////////////////////////////////
    // Protected
    ////////////////////////////////////////////////////////////////////////////

    protected abstract void playImpl( InputStream is, int expectedKBitSecRate ) throws Exception;


    protected void dumpHeaders( URLConnection cn ) {
        for (java.util.Map.Entry<String, java.util.List<String>> me : cn.getHeaderFields().entrySet()) {
            for (String s : me.getValue()) {
                Log.d( LOG, "header: key=" + me.getKey() + ", val=" + s);
                
            }
        }
    }


    protected int computeAvgKBitSecRate( Decoder.Info info ) {
        // do not change the value after a while - avoid changing of the out buffer:
        if (countKBitSecRate < 64) {
            int kBitSecRate = computeKBitSecRate( info );
            int frames = info.getRoundFrames();

            sumKBitSecRate += kBitSecRate * frames;
            countKBitSecRate += frames;
        }

        return sumKBitSecRate / countKBitSecRate;
    }


    protected static int computeKBitSecRate( Decoder.Info info ) {
        if (info.getRoundSamples() <= 0) return -1;

        return computeKBitSecRate( info.getRoundBytesConsumed(), info.getRoundSamples(),
                                   info.getSampleRate(), info.getChannels());
    }


    protected static int computeKBitSecRate( int bytesconsumed, int samples, int sampleRate, int channels ) {
        long ret = 8L * bytesconsumed * channels * sampleRate / samples;

        return (((int)ret) + 500) / 1000;
    }


    protected static int computeInputBufferSize( int kbitSec, int durationMs ) {
        return kbitSec * durationMs / 8;
    }


    protected static int computeInputBufferSize( Decoder.Info info, int durationMs ) {

        return computeInputBufferSize( info.getRoundBytesConsumed(), info.getRoundSamples(),
                                        info.getSampleRate(), info.getChannels(), durationMs );
    }


    protected static int computeInputBufferSize( int bytesconsumed, int samples,
                                                 int sampleRate, int channels, int durationMs ) {

        return (int)(((long) bytesconsumed) * channels * sampleRate * durationMs  / (1000L * samples));
    }


    protected static int computeOutputBufferSize( int sampleRate, int channels, int durationMs ) {
        return sampleRate * channels * durationMs / 1000;
    }
}
