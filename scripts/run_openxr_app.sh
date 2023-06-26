#!/bin/sh

# Script used to run an openxr app (hello_xr) against the running monado-based Pluto RR server. 
# NOTE: one NEEDS to run an openxr app on running monado-based RR server in order for a 
# RR client (client/ ) to connect successfully and get frames.

PATH_TO_LINUX_STREAMING_REPO=/home/fredinfinite23/code/PlutoVR/linux-streaming/
PATH_TO_HELLO_XR=~/code/OpenXR-SDK-Source/build/linux_release/src/tests/hello_xr/

## Those reverse are VERY important with Quest2 in order to allow webrtc signalling on 127.0.0.1
adb reverse tcp:8080 tcp:8080;
adb reverse tcp:61943 tcp:61943;

# Make sure to put/forge an  active_runtime.json file sitting right next to libopenxr_monado.so
# Here's the file if you happen to be lazy : 
#
#{
#    "file_format_version": "1.0.0",
#    "runtime": {
#        "library_path": "./libopenxr_monado.so"
#    }
#}
#
XR_RUNTIME_JSON=$PATH_TO_LINUX_STREAMING_REPO/server/build/monado/src/xrt/targets/openxr/active_runtime.json $PATH_TO_HELLO_XR/hello_xr -G Vulkan2
