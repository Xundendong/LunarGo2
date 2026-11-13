#!/usr/bin/env bash
set -e
archive=LunarGo2-linux-x86_64.tar.gz
curl -fL --retry 3 -o "$archive" "https://github.com/Xundendong/LunarGo2/releases/latest/download/$archive"
tar -xzf "$archive"
exec ./LunarGo2-linux-x86_64/run.sh "$@"
