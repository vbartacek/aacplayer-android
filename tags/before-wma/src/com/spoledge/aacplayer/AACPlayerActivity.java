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

import android.app.Activity;
import android.app.AlertDialog;

import android.content.DialogInterface;
import android.content.Intent;

import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.View;

import android.widget.ArrayAdapter;
import android.widget.AutoCompleteTextView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.TextView;


/**
 * This is the main activity.
 */
public class AACPlayerActivity extends Activity implements View.OnClickListener, PlayerCallback {

    private History history;
    private AutoCompleteTextView urlView;
    private Button btnFaad2;
    private Button btnFFmpeg;
    private Button btnOpenCORE;
    private Button btnStop;
    private TextView txtStatus;
    private TextView txtPlayStatus;
    private EditText txtBufAudio;
    private EditText txtBufDecode;
    private ProgressBar progress;
    private Handler uiHandler;

    /**
     * Decoder features: FAAD | FFmpeg | OpenCORE
     */
    private int dfeatures;

    private AACPlayer aacPlayer;
    private AACFileChunkPlayer aacFileChunkPlayer;


    ////////////////////////////////////////////////////////////////////////////
    // PlayerCallback
    ////////////////////////////////////////////////////////////////////////////

    private boolean playerStarted;

    public void playerStarted() {
        uiHandler.post( new Runnable() {
            public void run() {
                txtBufAudio.setEnabled( false );
                txtBufDecode.setEnabled( false );
                btnFaad2.setEnabled( false );
                btnFFmpeg.setEnabled( false );
                btnOpenCORE.setEnabled( false );
                btnStop.setEnabled( true );

                txtPlayStatus.setText( R.string.text_buffering );
                progress.setProgress( 0 );
                progress.setVisibility( View.VISIBLE );

                playerStarted = true;
            }
        });
    }


    /**
     * This method is called periodically by PCMFeed.
     *
     * @param isPlaying false means that the PCM data are being buffered,
     *          but the audio is not playing yet
     *
     * @param audioBufferSizeMs the buffered audio data expressed in milliseconds of playing
     * @param audioBufferCapacityMs the total capacity of audio buffer expressed in milliseconds of playing
     */
    public void playerPCMFeedBuffer( final boolean isPlaying,
                                     final int audioBufferSizeMs, final int audioBufferCapacityMs ) {

        uiHandler.post( new Runnable() {
            public void run() {
                progress.setProgress( audioBufferSizeMs * progress.getMax() / audioBufferCapacityMs );
                if (isPlaying) txtPlayStatus.setText( R.string.text_playing );
            }
        });
    }


    public void playerStopped( final int perf ) {
        uiHandler.post( new Runnable() {
            public void run() {
                enableButtons();
                btnStop.setEnabled( false );
                txtBufAudio.setEnabled( true );
                txtBufDecode.setEnabled( true );
                txtStatus.setText( R.string.text_stopped );
                txtPlayStatus.setText( "" + perf + " %" );
                progress.setVisibility( View.INVISIBLE );

                playerStarted = false;
            }
        });
    }


    public void playerException( final Throwable t) {
        uiHandler.post( new Runnable() {
            public void run() {
                new AlertDialog.Builder( AACPlayerActivity.this )
                    .setTitle( R.string.text_exception )
                    .setMessage( t.toString())
                    .setNeutralButton( R.string.button_close,
                        new DialogInterface.OnClickListener() {
                            public void onClick( DialogInterface dialog, int id) {
                                dialog.cancel();
                            }
                        }
                     )
                    .show();

                txtStatus.setText( R.string.text_stopped );

                if (playerStarted) playerStopped( 0 );
            }
        });
    }


    ////////////////////////////////////////////////////////////////////////////
    // OnClickListener
    ////////////////////////////////////////////////////////////////////////////

    /**
     * Called when a view has been clicked.
     */
    public void onClick( View v ) {
        try {
            switch (v.getId()) {

                case R.id.view_main_button_faad2:
                    start( Decoder.DECODER_FAAD2 );
                    txtStatus.setText( R.string.text_using_FAAD2 );
                    break; 

                case R.id.view_main_button_ffmpeg:
                    start( Decoder.DECODER_FFMPEG );
                    txtStatus.setText( R.string.text_using_FFmpeg );
                    break; 

                case R.id.view_main_button_opencore:
                    start( Decoder.DECODER_OPENCORE );
                    txtStatus.setText( R.string.text_using_OpenCORE );
                    break; 

                /*
                case R.id.view_main_button_file:
                    stop();
                    aacFileChunkPlayer = new AACFileChunkPlayer( getUrl(), 8000, 8000 );
                    aacFileChunkPlayer.start();
                    txtStatus.setText( R.string.text_using_file_chunks );
                    break;
                */

                case R.id.view_main_button_stop:
                    stop();
                    break;
            }
        }
        catch (Exception e) {
            Log.e( "AACPlayerActivity", "exc" , e );
        }
    }


    ////////////////////////////////////////////////////////////////////////////
    // Protected
    ////////////////////////////////////////////////////////////////////////////

    @Override
    protected void onCreate( Bundle savedInstanceState ) {
        super.onCreate( savedInstanceState );

        setContentView( R.layout.main );

        btnFaad2 = (Button) findViewById( R.id.view_main_button_faad2 );
        btnFFmpeg = (Button) findViewById( R.id.view_main_button_ffmpeg );
        btnOpenCORE = (Button) findViewById( R.id.view_main_button_opencore );
        //Button b3 = (Button) findViewById( R.id.view_main_button_file );
        btnStop = (Button) findViewById( R.id.view_main_button_stop );

        urlView = (AutoCompleteTextView) findViewById( R.id.view_main_edit_url );
        txtStatus = (TextView) findViewById( R.id.view_main_text_what );
        txtPlayStatus = (TextView) findViewById( R.id.view_main_text_status );
        txtBufAudio = (EditText) findViewById( R.id.view_main_text_bufaudio );
        txtBufDecode = (EditText) findViewById( R.id.view_main_text_bufdecode );

        progress = (ProgressBar) findViewById( R.id.view_main_progress );

        txtBufAudio.setText( String.valueOf( AACPlayer.DEFAULT_AUDIO_BUFFER_CAPACITY_MS ));
        txtBufDecode.setText( String.valueOf( AACPlayer.DEFAULT_DECODE_BUFFER_CAPACITY_MS ));

        btnFaad2.setOnClickListener( this );
        btnFFmpeg.setOnClickListener( this );
        btnOpenCORE.setOnClickListener( this );
        //b3.setOnClickListener( this );
        btnStop.setOnClickListener( this );

        dfeatures = ArrayDecoder.getFeatures();

        enableButtons();

        history = new History( this );
        history.read();

        if (history.size() == 0 ) {
            history.addUrl( "/sdcard/local/cro2-32.aac" );
            history.addUrl( "http://netshow.play.cz:8000/crocb32aac" );
            history.addUrl( "http://62.44.1.26:8000/cro2-128aac" );
            history.addUrl( "http://2483.live.streamtheworld.com:80/KFTZFMAACCMP3" );
            history.addUrl( "http://http.yourmuze.com:8000/play/paradise/l.aac" );
            history.addUrl( "http://http.yourmuze.com:8000/play/paradise/m.aac" );
            history.addUrl( "http://http.yourmuze.com:8000/play/paradise/h.aac" );
        }

        urlView.setAdapter( history.getArrayAdapter());
        uiHandler = new Handler();
    }


    @Override
    protected void onPause() {
        super.onPause();
        history.write();
    }


    @Override
    protected void onDestroy() {
        super.onDestroy();
        stop();
    }


    ////////////////////////////////////////////////////////////////////////////
    // Private
    ////////////////////////////////////////////////////////////////////////////

    private void start( int decoder ) {
        stop();
        aacPlayer = new ArrayAACPlayer( ArrayDecoder.create( decoder ), this,
                                        getInt( txtBufAudio ), getInt( txtBufDecode ));
        aacPlayer.playAsync( getUrl());
    }


    private void stop() {
        if (aacFileChunkPlayer != null) { aacFileChunkPlayer.stop(); aacFileChunkPlayer = null; }
        if (aacPlayer != null) { aacPlayer.stop(); aacPlayer = null; }
    }


    private String getUrl() {
        String ret = urlView.getText().toString();

        history.addUrl( ret );

        return ret;
    }


    private void enableButtons() {
        if ((dfeatures & Decoder.DECODER_FAAD2) != 0) btnFaad2.setEnabled( true );
        if ((dfeatures & Decoder.DECODER_FFMPEG) != 0) btnFFmpeg.setEnabled( true );
        if ((dfeatures & Decoder.DECODER_OPENCORE) != 0) btnOpenCORE.setEnabled( true );
    }


    private int getInt( EditText et ) {
        return Integer.parseInt( et.getText().toString());
    }
}

