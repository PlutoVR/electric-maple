#!/bin/sh
# Copyright 2023, Collabora, Ltd.
#
# SPDX-License-Identifier: BSL-1.0

# Just stop the app.

# shellcheck source-path=SCRIPTDIR
# shellcheck source=common.sh
. ./common.sh
adb shell am force-stop "$PKG"
