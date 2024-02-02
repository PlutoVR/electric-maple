#!/bin/sh
# Copyright 2023, Collabora, Ltd.
#
# SPDX-License-Identifier: BSL-1.0

# Settings to source in various other scripts.

export PKG="com.plutovr.electricmaple.standalone_client"
export ACTIVITY=com.plutovr.electricmaple.standalone_client.StreamingActivity
#export ACTIVITY=android.app.NativeActivity

export LOGCAT_GREP_PATTERN="(ElectricMaple|[Gg][Ss]treamer|PlutoSphereClient|RYLIE|glib|DEBUG|soup)"
