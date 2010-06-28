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
import android.util.Log;
import android.view.View;

import android.widget.ArrayAdapter;
import android.widget.AutoCompleteTextView;
import android.widget.Button;
import android.widget.EditText;


/**
 * This is the main activity.
 */
public class AACPlayerActivity extends Activity implements View.OnClickListener {

    private History history;
    private AutoCompleteTextView urlView;

    private AACPlayer aacPlayer;
    private AACFileChunkPlayer aacFileChunkPlayer;


    ////////////////////////////////////////////////////////////////////////////
    // OnClickListener
    ////////////////////////////////////////////////////////////////////////////

    /**
     * Called when a view has been clicked.
     */
    public void onClick( View v ) {
        try {
            switch (v.getId()) {
                case R.id.view_main_button_native:
                    stop();
                    aacPlayer = new AACPlayer();
                    aacPlayer.playAsync( getUrl(), AACPlayer.Quality.LOW_32 );
                    break; 

                case R.id.view_main_button_file:
                    stop();
                    aacFileChunkPlayer = new AACFileChunkPlayer( getUrl(), 8000, 8000 );
                    aacFileChunkPlayer.start();
                    break;

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

        Button b1 = (Button) findViewById( R.id.view_main_button_native );
        Button b2 = (Button) findViewById( R.id.view_main_button_file );
        Button b3 = (Button) findViewById( R.id.view_main_button_stop );

        urlView = (AutoCompleteTextView) findViewById( R.id.view_main_edit_url );

        b1.setOnClickListener( this );
        b2.setOnClickListener( this );
        b3.setOnClickListener( this );

        history = new History( this );
        history.read();

        if (history.size() == 0 ) {
            history.addUrl( "/sdcard/local/cro2-32.aac" );
            history.addUrl( "http://netshow.play.cz:8000/crocb32aac" );
        }

        urlView.setAdapter( history.getArrayAdapter());
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

