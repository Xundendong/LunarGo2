#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"
exec env -u LD_LIBRARY_PATH ./LunarG1/Binaries/Linux/LunarG1 -windowed -ResX=1280 -ResY=720 -nosplash -norhithread -ansimalloc "$@"
