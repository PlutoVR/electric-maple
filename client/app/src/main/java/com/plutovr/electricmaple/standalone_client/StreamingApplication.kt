// Copyright 2023, Pluto VR, Inc.
//
// SPDX-License-Identifier: BSL-1.0

package com.plutovr.electricmaple.standalone_client

import android.app.Application;
import android.util.Log
import org.freedesktop.gstreamer.GStreamer

class StreamingApplication : Application() {
    override fun onCreate() {
        super.onCreate()
        Log.i("ElectricMaple", "StreamingApplication: In onCreate")
        System.loadLibrary("gstreamer_android")
        Log.i("ElectricMaple", "StreamingApplication: loaded gstreamer_android")

        Log.i("ElectricMaple", "StreamingApplication: Calling GStreamer.init")
        GStreamer.init(this)
        Log.i("ElectricMaple", "StreamingApplication: Done with GStreamer.init")

    }
}
