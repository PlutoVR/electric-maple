#!/bin/sh

# Copyright 2023, Pluto VR, Inc.
#
# SPDX-License-Identifier: BSL-1.0

# Script used to run an openxr app (hello_xr) against the running monado-based Electric Maple RR server.
# NOTE: one NEEDS to run an openxr app on running monado-based RR server in order for a
# RR client (client/ ) to connect successfully and get frames.
set -e
EM_ROOT=$(cd "$(dirname "$0")" && cd .. && pwd)
HELLOXR=${HELLOXR:-~/code/OpenXR-SDK-Source/build/linux_release/src/tests/hello_xr/hello_xr}


env XR_RUNTIME_JSON="$EM_ROOT/server/build/openxr_monado-dev.json" $HELLOXR -G Vulkan2
