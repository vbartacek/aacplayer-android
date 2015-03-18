# Freeware Advanced Audio (AAC) Player for Android #
http://www.spoledge.com

**NOTE:** this project was a Proof of Concept only. If you are looking for an AAC decoder library which you would like to use in your own projects, then please visit the follow-up project http://code.google.com/p/aacdecoder-android/.

Please see also my [blog](http://vaclavb.blogspot.com/2010/06/raw-aac-player-for-android.html).

This is a port of the following open source libraries to Android platform:
  * [FAAD2](http://www.audiocoding.com/) (Freeware Advanced Audio (AAC) Decoder including SBR decoding)
  * [FFmpeg](http://ffmpeg.org) library (only AAC decoder)
  * [OpenCORE aacdec](http://code.google.com/p/opencore-aacdec/) library

This project was written for those developers spending hours of googling the internet to answer a simple question:
> How to make Android to **play raw AAC files (streams)** ?

I spent many hours by it and now I want to save other people's time - so I am publishing what I have found.

I tried to:
  * invoke `MediaPlayer` on raw AAC `http://` URL (error)
  * invoke `MediaPlayer` on raw AAC file (error in emulator, OK on HTC Desire)
  * pass a `NetSocket` `FileHandle` to `MediaPlayer` (error)
  * filter the stream using local `ServerSocket` and:
    * discard HTTP header (error)
    * change content-type to several possibilities like audio/mpeg (error)
    * sync ADTS header (icecast does not do it) (error)
  * split the AAC stream into small file chunks (OK, but not excellent)
  * use 3rd party native libraries (FAAD2/FFmpeg) to decode AAC (OK on HTC Desire, working on emulator - but slow)
  * finally using the 3rd party library **OpenCORE aacdec** met the expectations - and playing is smooth even on emulator


---

This software uses [FAAD2](http://www.audiocoding.com/) library to decode AAC streams.
For more information about FAAD2, please visit http://www.audiocoding.com/

This software uses code of [FFmpeg](http://ffmpeg.org) licensed under the [LGPLv2.1](http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html) and its source can be downloaded [here](http://code.google.com/p/aacplayer-android/downloads/list).
For more information about FFmpeg, please visit http://ffmpeg.org/

This software uses code of [OpenCORE aacdec](http://code.google.com/p/opencore-aacdec/) licensed under the [Apache License, Version 2.0](http://www.apache.org/licenses/) and its source can be downloaded [here](http://code.google.com/p/aacplayer-android/downloads/list).
For more information about OpenCORE aacdec, please visit http://code.google.com/p/opencore-aacdec/


**PLEASE NOTE**
that the use of this software may require the payment of
patent royalties. You need to consider this issue before you start
building derivative works. We are not warranting or indemnifying you in
any way for patent royalities! YOU ARE SOLELY RESPONSIBLE FOR YOUR OWN
ACTIONS!

For more information about the AAC patents, please visit
http://www.vialicensing.com/licensing/AAC_index.cfm