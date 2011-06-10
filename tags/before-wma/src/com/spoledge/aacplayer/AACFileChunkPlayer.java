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

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import java.net.URL;
import java.net.URLConnection;

import android.media.MediaPlayer;
import android.os.Handler;
import android.util.Log;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.ProgressBar;
import android.widget.TextView;


/**
 * Plays AAC stream (ADTS) by splitting it to file chunks.
 */
public class AACFileChunkPlayer implements MediaPlayer.OnCompletionListener, MediaPlayer.OnErrorListener {
    private static final String LOG = "Streamer";

    private Downloader downloader;
    private MediaPlayer mediaPlayer;
    private File file;

    private int initBufferSize;
    private int minBufferSize;
    private boolean isRunning;

    private final Handler handler = new Handler();


    ////////////////////////////////////////////////////////////////////////////
    // Constructors
    ////////////////////////////////////////////////////////////////////////////

    public AACFileChunkPlayer( String url, int initBufferSize, int minBufferSize ) {
        this.downloader = new Downloader( url );
        this.initBufferSize = initBufferSize;
        this.minBufferSize = minBufferSize;
    }


    ////////////////////////////////////////////////////////////////////////////
    // Public
    ////////////////////////////////////////////////////////////////////////////

    public void start() throws IOException {
        Log.d( LOG, "starting...");
        isRunning = true;

        downloader.start();
        Log.d( LOG, "downloader started.");

        play( initBufferSize );
    }


    public void stop() {
        Log.d( LOG, "stopping...");

        stopMediaPlayer( true );
        downloader.stop();

        Log.d( LOG, "stopped.");
    }


    ////////////////////////////////////////////////////////////////////////////
    // MediaPlayer.OnCompletionListener
    ////////////////////////////////////////////////////////////////////////////

    public void onCompletion( MediaPlayer mp ) {
        Log.d( LOG, "onCompletion()");

        if (stopMediaPlayer( false )) return;

        play( minBufferSize );
    }


    ////////////////////////////////////////////////////////////////////////////
    // MediaPlayer.OnErrorListener
    ////////////////////////////////////////////////////////////////////////////

    public boolean onError( MediaPlayer mp, int what, int extra ) {
        Log.d( LOG, "onError()");

        /*
        if (what == 1) {
            if (extra == 13) return true;
        }
        */

        synchronized (this) {
            if (mediaPlayer != null) {
                mediaPlayer.release();
                mediaPlayer = null;
            }

            isRunning = false;
        }

        downloader.stop();

        return true;
    }


    ////////////////////////////////////////////////////////////////////////////
    // Private
    ////////////////////////////////////////////////////////////////////////////

    private void play( final int minBytes ) {
        Log.d( LOG, "play()");

        handler.post( new Runnable() {
            public void run() {
                try {
                    playImpl( minBytes );
                }
                catch (IOException e) {
                    stop();
                }
            }
        });
    }


    private void playImpl( int minBytes ) throws IOException {
        Log.d( LOG, "getting file...");
        file = downloader.getFile( minBytes, true );

        Log.d( LOG, "got file: " + file);

        if (file == null) return;

        MediaPlayer mp = new MediaPlayer();
        mp.setOnErrorListener( this );
        mp.setDataSource( file.getAbsolutePath());
        mp.prepare();

        mp.setOnCompletionListener( this );

        mp.start();

        mediaPlayer = mp;
    }


    private synchronized boolean stopMediaPlayer( boolean isForced ) {
        synchronized (this) {
            if (mediaPlayer != null) {
                mediaPlayer.stop();
                mediaPlayer.release();
                mediaPlayer = null;
            }

            if (file != null) {
                file.delete();
                file = null;
            }

            if (isForced) isRunning = false;

            return !isRunning;
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // Inner classes
    ////////////////////////////////////////////////////////////////////////////

    private static class Downloader implements Runnable {
        enum State { IDLE, STARTED, RUNNING, FLUSHING, FLUSHED, FINISHED, STOPPED };

        State state = State.IDLE;

        String url;
        InputStream in;

        File file;
        OutputStream out;

        int bytes = 0;
        int totalBytes = 0;


        Downloader( String url ) {
            this.url = url;
        }


        public synchronized void start() throws IOException {
            if (state != State.IDLE) throw new IllegalStateException("Cannot start() in state " + state);
            state = State.STARTED;

            URLConnection cn = new URL( url ).openConnection();   
            cn.connect();
            in = cn.getInputStream();

            if (in == null) {
                Log.e( LOG, "Cannot open stream for URL: " + url);

                throw new IOException("empty stream");
            }

            createOutput();

            new Thread( this ).start();
        }


        public synchronized void stop() {
            if (state != State.IDLE) state = State.STOPPED;

            // in case there exists a waiting thread:
            notify();
        }


        /**
         * Returns the downloaded part.
         * This method can be called multiple times.
         */
        public synchronized File getFile( int minBytes, boolean block ) throws IOException {
            while (bytes < minBytes && state.ordinal() < State.FINISHED.ordinal()) {
                Log.d( LOG, "getFile(): bytes=" + bytes + " < minBytes=" + minBytes );

                if (block) try { wait();} catch (InterruptedException e) {}
                else return null;
            }

            switch (state) {
                case STARTED:
                case RUNNING:
                    state = State.FLUSHING;
                    // no break here

                case FLUSHING:
                    if (!block) return null;

                    while (state == State.FLUSHING) {
                        try { wait();} catch (InterruptedException e) {}
                    }

                    if (state != State.FLUSHED) return null;
                    // no break here

                case FLUSHED:
                    if (out != null) try { out.close();} catch (IOException e){}
                    File ret = file;
                    createOutput();

                    state = State.RUNNING;
                    notify();

                    return ret;

                case FINISHED:
                    ret = file;
                    file = null;

                    return ret;
            }

            return null;
        }


        public void run() {
            byte[] buf = new byte[16384];

            try {
                int[] ref = new int[2];

                while (true) {
                    int n = in.read( buf );

                    synchronized (this) {
                        if (state == State.STOPPED) break;
                        if (n <= 0) {
                            state = State.FINISHED;
                            break;
                        }

                        int off = 0;

                        if (state == State.FLUSHING) {
                            ref[ 1 ] = n;

                            if (!aacSync( buf, ref, true )) {
                                Log.e( LOG, "run(): cannot sync AAC stream" );
                                state = State.FINISHED;
                                break;
                            }

                            // flush the AAC frame:
                            n = ref[ 0 ];
                            out.write( buf, 0, n );
                            bytes += n;
                            totalBytes += n;

                            state = State.FLUSHED;
                            notify();
                            try { wait(); } catch (InterruptedException e) {}

                            // write the next AAC frame:

                            off = ref[ 0 ];
                            n = ref[ 1 ] - off;
                        }
                        else if (state == State.STARTED) {
                            ref[ 1 ] = n;

                            if (!aacSync( buf, ref, true )) {
                                Log.e( LOG, "run(): cannot sync AAC stream" );
                                state = State.FINISHED;
                                break;
                            }

                            off = ref[ 0 ];
                            n = ref[ 1 ] - off;
                        }

                        out.write( buf, off, n );
                        bytes += n;
                        totalBytes += n;

                        // wake-up waiting thread:
                        notify();
                    }

                    Log.d( LOG, String.valueOf( bytes ) + " bytes read");
                }
            }
            catch (IOException e) {
                Log.e( LOG, "run()", e );
            }
            finally {
                if (in != null) try { in.close();} catch (IOException e) {}
                in = null;

                synchronized (this) {
                    if (out != null) try { out.close();} catch (IOException e) {}
                    out = null;
                    if (file != null) file.delete();
                    file = null;

                    notify();
                }
            }
        }


        private void createOutput() throws IOException {
            file = File.createTempFile( "streamer-downloading", ".aac");
            out = new FileOutputStream( file );   
            bytes = 0;
        }


        /**
         * Tries to sync the ADTS stream.
         *
         * @param ref 2 elements array: [ offset, length ]: offset of the ADTS header,
         *      length the actual length of data read
         * @param drop whether to drop bytes before the ADTS header
         * @return true iff the sync was successful
         */
        private boolean aacSync( byte[] buf, int[] ref, boolean drop ) throws IOException {
            int n = ref[ 1 ];
            int maxBytes = 4096; // if the header is not found here, then abort

            while (n >= 0 && state != State.STOPPED) {
                for (int i=0; i < n-4; i++) {
                    int b1 = buf[i] & 0xff;
                    int b2 = buf[i+1] & 0xff;

                    if (b1 == 0xff && (b2 & 0xf6) == 0xf0) {
                        ref[ 0 ] = i;
                        ref[ 1 ] = n;

                        return true;
                    }
                }

                Log.i( LOG, "aacSync(): cannot find ADTS header, " + (drop ? "drop" : "skip") + "ing " + n + " bytes");

                if (!drop) {
                    out.write( buf, 0, n );
                    bytes += n;
                    totalBytes += n;
                }

                maxBytes -= n;

                if (maxBytes <= 0) break;

                n = in.read( buf );
            }

            return false;
        }

    }

}
