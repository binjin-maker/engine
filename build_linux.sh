#!/bin/sh
set -e
[ -d raylib ] || git clone --depth 1 https://github.com/raysan5/raylib.git
[ -f raylib/lin/libraylib.a ] || (mkdir -p raylib/lin && cd raylib/src && make PLATFORM=PLATFORM_DESKTOP RAYLIB_RELEASE_PATH=../lin && make clean)
python3 tools/embed_fonts.py
gcc -O2 -Iraylib/src main.c raylib/lin/libraylib.a -lGL -lm -lpthread -ldl -lrt -lX11 -o engine
