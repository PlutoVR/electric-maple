#!/bin/sh
# Copyright 2023, Collabora, Ltd.
#
# SPDX-License-Identifier: BSL-1.0

# Stop the app, find the process ID through knowing a unique log tag,
# then dump the entirety of that process's log.
# We also make a "clean" version for use in diffs.

set -e
# shellcheck source-path=SCRIPTDIR
# shellcheck source=common.sh
. ./common.sh

EM_PID=$(adb logcat -d |grep -E "$LOGCAT_GREP_PATTERN" | awk {'print $3'} | head -n1)

TIMESTAMP=$(date --iso-8601=seconds | sed 's/:/_/g')

FN="logcat.$TIMESTAMP.txt"

echo "$FN"

adb logcat -d |grep "$EM_PID" > "$FN"

./clean-logcat.sh "$FN"

# ./compare-line-numbers.sh "$CLEAN_FN"

./stop.sh
