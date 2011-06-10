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

import android.util.Log;

import java.io.InputStream;
import java.io.IOException;


/**
 * This is a MMS input stream - reads data from a MMS stream.
 * <pre>
 *  MMSInputStream mmsis = new MMSInputStream( "mms://..." );
 *  ...
 *  mmsis.read( buffer );
 *  ...
 *  mmsis.close();
 * </pre>
 */
public class MMSInputStream extends InputStream {

    private static final String LOG = "MMSInputStream"; 

    private static boolean libLoaded = false;


    ////////////////////////////////////////////////////////////////////////////
    // Attributes
    ////////////////////////////////////////////////////////////////////////////

    /**
     * The native handler.
     */
    private int mms;


    ////////////////////////////////////////////////////////////////////////////
    // Constructors
    ////////////////////////////////////////////////////////////////////////////

    public MMSInputStream( String url ) throws IOException {
        ensureLibLoaded();

        mms = nativeConnect( url );
    }


    ////////////////////////////////////////////////////////////////////////////
    // Public - InputStream
    ////////////////////////////////////////////////////////////////////////////

    /**
     * Please do not use this method - this is not efficient !
     */
    public int read() throws IOException {
        byte[] buf = new byte[1];

        int n;
        while ((n = read( buf, 0, 1)) == 0) {
        }

        if (n == 1) return ((int)buf[0]) & 0xff;
        return n;
    }


    @Override
    public int read(byte[] b, int off, int len) throws IOException {
        return nativeRead( mms, b, off, len );
    }


    @Override
    public void close() throws IOException {
        nativeClose( mms );
    }


    ////////////////////////////////////////////////////////////////////////////
    // Private
    ////////////////////////////////////////////////////////////////////////////

    /**
     * This is a conversion method used by libmms.
     */
    private static synchronized void ensureLibLoaded() {
        if (!libLoaded) {
            System.loadLibrary( "mms" );
            libLoaded = true;
        }
    }


    private static final String CHARSET_NAME_UNICODE = "UTF-16LE";

    /**
     * This is a conversion method used by libmms.
     */
    private static byte[] string2unicode( String s ) {
        Log.d( LOG, "string2unicode(): '" + s  + "'" );

        // return s.getBytes( CHARSET_UNICODE );

        try {
            return s.getBytes( CHARSET_NAME_UNICODE );
        }
        catch (java.io.UnsupportedEncodingException e) {
            Log.e( LOG, "Cannot convert string --> unicode", e );
            throw new RuntimeException( e );
        }
    }

    /**
     * This is a conversion method used by libmms.
     */
    private static String unicode2string( byte[] bytes ) {
        try {
            String ret = new String( bytes, CHARSET_NAME_UNICODE );
            Log.d( LOG, "unicode2string(): '" + ret  + "'" );

            return ret;
        }
        catch (java.io.UnsupportedEncodingException e) {
            Log.e( LOG, "Cannot convert unicode --> string", e );
            throw new RuntimeException( e );
        }
    }


    private native int nativeConnect( String url ) throws IOException;
    private native int nativeRead( int mms, byte[] b, int off, int len ) throws IOException;
    private native void nativeClose( int mms ) throws IOException;

}
