package com.plutovr.plutosphere.webrtc_client
import org.freedesktop.gstreamer.GStreamer

import android.app.NativeActivity
import android.os.Bundle
import android.os.PersistableBundle

class StreamingActivity: NativeActivity() {
    override fun onCreate(savedInstanceState: Bundle?, persistentState: PersistableBundle?) {
        super.onCreate(savedInstanceState, persistentState)
        GStreamer.init(this)
    }
    init {

    }
}