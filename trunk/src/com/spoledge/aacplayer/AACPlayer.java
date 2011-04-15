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
 */
public abstract class AACPlayer {

    private static final String LOG = "AACPlayer";


    ////////////////////////////////////////////////////////////////////////////
    // Attributes
    ////////////////////////////////////////////////////////////////////////////

    protected boolean stopped;


    ////////////////////////////////////////////////////////////////////////////
    // Constructors
    ////////////////////////////////////////////////////////////////////////////

    public AACPlayer() {
    }


    ////////////////////////////////////////////////////////////////////////////
    // Public
    ////////////////////////////////////////////////////////////////////////////

    public void playAsync( final String url, final Decoder decoder, final PlayerCallback clb ) {
        new Thread(new Runnable() {
            public void run() {
                try {
                    play( url, decoder, clb );
                }
                catch (Exception e) {
                    Log.e( LOG, "playAsync():", e);

                    if (clb != null) clb.playerException( e );
                }
            }
        }).start();
    }


    public abstract void play( String url, Decoder decoder, PlayerCallback clb ) throws Exception;


    public void stop() {
        stopped = true;
    }


    ////////////////////////////////////////////////////////////////////////////
    // Protected
    ////////////////////////////////////////////////////////////////////////////

    protected void dumpHeaders( URLConnection cn ) {
        for (java.util.Map.Entry<String, java.util.List<String>> me : cn.getHeaderFields().entrySet()) {
            for (String s : me.getValue()) {
                Log.d( LOG, "header: key=" + me.getKey() + ", val=" + s);
                
            }
        }
    }
}
