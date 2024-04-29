// Copyright 2023, Pluto VR, Inc.
//
// SPDX-License-Identifier: BSL-1.0

package com.plutovr.electricmaple.standalone_client

import org.freedesktop.gstreamer.GStreamer

import android.app.NativeActivity
import android.os.Bundle
import android.os.PersistableBundle
import android.util.Log

class StreamingActivity : NativeActivity() {
    override fun onCreate(savedInstanceState: Bundle?, persistentState: PersistableBundle?) {

        System.loadLibrary("electricmaple_client")
        Log.i("ElectricMaple", "StreamingActivity: loaded electricmaple_client")
        System.loadLibrary("electricmaple_standalone_client")
        Log.i("ElectricMaple", "StreamingActivity: loaded")
        super.onCreate(savedInstanceState, persistentState)
    }

    companion object {
        init {
            Log.i("ElectricMaple", "StreamingActivity: In StreamingActivity static init")


        }
    }
}
