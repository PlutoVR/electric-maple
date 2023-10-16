#!/usr/bin/env bash
# Copyright 2023, Collabora, Ltd.
#
# SPDX-License-Identifier: BSL-1.0

# Strip out stuff from logcat files that always varies between runs (times/pid/tid),
# plus the default log tag (ElectricMaple)
# Makes it easier to diff logcat files.

set -e

if [ "$1" = "" ]; then
    echo "Please pass logcat file name!"
    exit 1
fi

FN=$1
CLEAN_FN="${FN%%.txt}.clean.txt"

awk '{ $1 = ""; $2 = ""; $3 = ""; $4 = "";  print }' < "$FN" | \
    sed -E 's/0:00:([0-9]+)[.]([0-9]+) ([0-9]+) 0x([0-9a-z]+) //g' | \
    sed -E 's/(ElectricMaple)://' > "$CLEAN_FN"
