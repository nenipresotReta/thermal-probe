#!/bin/sh
set -eu
APP=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
python3 "$APP/tools/configure.py"
exec west build -s "$APP" -d "$APP/build" -b tdisplay_s3/esp32s3/procpu "$@"
