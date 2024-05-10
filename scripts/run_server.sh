#!/bin/sh

# Copyright 2023, Pluto VR, Inc.
#
# SPDX-License-Identifier: BSL-1.0

EM_ROOT=$(cd "$(dirname "$0")" && cd .. && pwd)

"$EM_ROOT/server/build/src/ems/ems_streaming_server"
