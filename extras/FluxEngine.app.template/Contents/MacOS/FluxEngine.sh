#!/bin/sh
dir=`dirname "$0"`
cd "$dir"
export DYLD_FALLBACK_FRAMEWORK_PATH=../Resources
exec ./fluxengine-gui "$@"

