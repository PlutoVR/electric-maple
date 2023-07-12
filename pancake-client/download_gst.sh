#!/bin/bash

# Go to where the script is
cd $(dirname $0)

# Create the destination directory if it doesn't exist
mkdir -p ./deps/gstreamer_android

# Download the file using curl
curl -L -o ./deps/gstreamer_android/gstreamer-1.0-android-universal-1.22.3.tar.xz https://gstreamer.freedesktop.org/data/pkg/android/1.22.3/gstreamer-1.0-android-universal-1.22.3.tar.xz

# Extract the contents of the downloaded file to the destination directory
tar -xf ./deps/gstreamer_android/gstreamer-1.0-android-universal-1.22.3.tar.xz -C ./deps/gstreamer_android

rm ./deps/gstreamer_android/gstreamer-1.0-android-universal-1.22.3.tar.xz
