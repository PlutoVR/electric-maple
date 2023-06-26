#!/bin/sh

PATH_TO_LINUX-STREAMING_ROOT_FOLDER=~/code/PlutoVR/linux-streaming

# Script to run the server
cd $PATH_TO_LINUX-STREAMING_ROOT_FOLDER/server/build/src/pluto; rm -fR /run/user/1000/monado_comp_ipc; ./pluto_streaming_server
