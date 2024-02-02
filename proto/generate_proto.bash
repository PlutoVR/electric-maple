#!/bin/sh
# Copyright 2023, Pluto VR, Inc.
# SPDX-License-Identifier: BSL-1.0
# Author: Moshi Turner <moses.turner@collabora.com>

set -e

cd "$(dirname "$0")"

echo "at $(pwd)"

if ! command -v nanopb_generator > /dev/null; then
    echo "nanopb_generator not found, do pipx install nanopb"
    exit 1
fi

mkdir -p generated

nanopb_generator electricmaple.proto
mv electricmaple.pb.h electricmaple.pb.c generated/
