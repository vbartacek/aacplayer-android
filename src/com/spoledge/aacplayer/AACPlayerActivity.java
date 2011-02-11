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

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.View;

import android.widget.ArrayAdapter;
import android.widget.AutoCompleteTextView;
import android.widget.Button;
import android.widget.EditText;


/**
 * This is the main activity.
 */
public class AACPlayerActivity extends Activity implements View.OnClickListener, PlayerCallback {

    private History history;
    private AutoCompleteTextView urlView;
    private Button btnFaad2;
    private Button btnFFmpeg;
    private Button btnStop;
    private Handler uiHandler;

    private DirectAACPlayer aacPlayer;
    private AACFileChunkPlayer aacFileChunkPlayer;


    ////////////////////////////////////////////////////////////////////////////
    // PlayerCallback
    ////////////////////////////////////////////////////////////////////////////

    public void playerStarted() {
        uiHandler.post( new Runnable() {
            public void run() {
                btnFaad2.setEnabled( false );
                btnFFmpeg.setEnabled( false );
                btnStop.setEnabled( true );
            }
        });
    }


    public void playerDataRead( int bytes ) {
    }


    public void playerStopped() {
        uiHandler.post( new Runnable() {
            public void run() {
                btnFaad2.setEnabled( true );
                btnFFmpeg.setEnabled( true );
                btnStop.setEnabled( false );
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
                    stop();
                    aacPlayer = new DirectAACPlayer();
                    aacPlayer.playAsync( getUrl(), FAADDecoder.create(), this );
                    break; 

                case R.id.view_main_button_ffmpeg:
                    stop();
                    aacPlayer = new DirectAACPlayer();
                    aacPlayer.playAsync( getUrl(), FFMPEGDecoder.create(), this );
                    break; 

                /*
                case R.id.view_main_button_file:
                    stop();
                    aacFileChunkPlayer = new AACFileChunkPlayer( getUrl(), 8000, 8000 );
                    aacFileChunkPlayer.start();
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
        //Button b3 = (Button) findViewById( R.id.view_main_button_file );
        btnStop = (Button) findViewById( R.id.view_main_button_stop );

        urlView = (AutoCompleteTextView) findViewById( R.id.view_main_edit_url );

        btnFaad2.setOnClickListener( this );
        btnFFmpeg.setOnClickListener( this );
        //b3.setOnClickListener( this );
        btnStop.setOnClickListener( this );

        history = new History( this );
        history.read();

        if (history.size() == 0 ) {
            history.addUrl( "/sdcard/local/cro2-32.aac" );
            history.addUrl( "http://netshow.play.cz:8000/crocb32aac" );
            history.addUrl( "http://2483.live.streamtheworld.com:80/KFTZFMAACCMP3" );
        }

        urlView.setAdapter( history.getArrayAdapter());
        uiHandler = new Handler();
    }


    @Override
    protected void onPause() {
        super.onPause();
        history.write();
    }


    ////////////////////////////////////////////////////////////////////////////
    // Private
    ////////////////////////////////////////////////////////////////////////////

    private void stop() {
        if (aacFileChunkPlayer != null) { aacFileChunkPlayer.stop(); aacFileChunkPlayer = null; }
        if (aacPlayer != null) { aacPlayer.stop(); aacPlayer = null; }
    }


    private String getUrl() {
        String ret = urlView.getText().toString();

        history.addUrl( ret );

        return ret;
    }
}
