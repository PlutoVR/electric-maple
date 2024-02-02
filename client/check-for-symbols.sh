#!/usr/bin/env bash
# Copyright 2023, Collabora, Ltd.
#
# SPDX-License-Identifier: BSL-1.0

# Verify we have the Java and native code symbols we expect in the places we expect
# for gstreamer on Android to work right.

REL_PATH=app/build/intermediates/stripped_native_libs/debug/out/lib/arm64-v8a

RESULT=true

for sym in JNI_OnLoad JNI_OnUnload gst_android_get_application_class_loader; do
    SO_FILE=$REL_PATH/libgstreamer_android.so
    nm -nDC $SO_FILE | grep "$sym"

    if nm -nDC $SO_FILE | grep -q "$sym"; then
        # echo "$sym:  FOUND"
        echo
    else
        echo "$sym:  MISSING from $SO_FILE!"
        RESULT=false
    fi
done


CLASS_ROOT=gstreamer_java/build/intermediates/javac/debug/classes
CLASS_FILES+=("$CLASS_ROOT/org/freedesktop/gstreamer/GStreamer.class")
CLASS_FILES+=("$CLASS_ROOT/org/freedesktop/gstreamer/androidmedia/GstAmcOnFrameAvailableListener.class")

for classfile in "${CLASS_FILES[@]}"; do

    if [ -f "$classfile" ]; then
        echo "$classfile"
    else
        echo "MISSING: $classfile"
        RESULT=false
    fi
done

$RESULT
