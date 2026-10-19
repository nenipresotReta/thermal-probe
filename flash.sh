#!/bin/sh
set -eu
APP=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
# Always validate and rebuild: never flash a stale image after changing config.
sh "$APP/build.sh"
exec west flash -d "$APP/build" "$@"
