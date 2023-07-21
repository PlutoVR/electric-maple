package com.plutovr.plutosphere.webrtc_client

import org.freedesktop.gstreamer.GStreamer

import android.app.NativeActivity
import android.os.Bundle
import android.os.PersistableBundle
import android.util.Log

class StreamingActivity : NativeActivity() {
    override fun onCreate(savedInstanceState: Bundle?, persistentState: PersistableBundle?) {

        System.loadLibrary("pluto_vf")
        Log.i("ElectricMaple", "StreamingActivity: loaded pluto_vf")
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
