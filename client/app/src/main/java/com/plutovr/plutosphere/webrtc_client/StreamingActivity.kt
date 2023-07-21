package com.plutovr.plutosphere.webrtc_client

import org.freedesktop.gstreamer.GStreamer

import android.app.NativeActivity
import android.os.Bundle
import android.os.PersistableBundle
import android.util.Log

class StreamingActivity : NativeActivity() {
    override fun onCreate(savedInstanceState: Bundle?, persistentState: PersistableBundle?) {
        Log.i("ElectricMaple", "StreamingActivity: In onCreate")
        System.loadLibrary("pluto_vf")
        Log.i("ElectricMaple", "StreamingActivity: loaded pluto_vf")
        Log.i("ElectricMaple", "StreamingActivity: Calling GStreamer.init")
        GStreamer.init(this)
        Log.i("ElectricMaple", "StreamingActivity: Done with GStreamer.init")
        System.loadLibrary("plutovr_webrtc_client")
        Log.i("ElectricMaple", "StreamingActivity: loaded")
        super.onCreate(savedInstanceState, persistentState)
    }

    companion object {
        init {
            Log.i("ElectricMaple", "StreamingActivity: In StreamingActivity static init")


        }
    }
}