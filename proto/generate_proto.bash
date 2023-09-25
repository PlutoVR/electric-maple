#!/bin/sh
set -e

cd "$(dirname "$0")"

echo "at $(pwd)"

if ! command -v nanopb_generator > /dev/null; then
    echo "nanopb_generator not found, do pipx install nanopb"
    exit 1
fi

mkdir -p generated

nanopb_generator pluto.proto
mv pluto.pb.h pluto.pb.c generated/
