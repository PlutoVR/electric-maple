#!/bin/sh

EM_ROOT=$(cd "$(dirname "$0")" && cd .. && pwd)

"$EM_ROOT/server/build/src/ems/ems_streaming_server"
