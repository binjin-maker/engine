#!/bin/sh
# Cross-compile a static Windows exe from Linux (needs mingw-w64 and a Windows raylib build in ./raylib)
set -e
[ -d raylib ] || git clone --depth 1 https://github.com/raysan5/raylib.git
[ -f raylib/win/libraylib.a ] || (mkdir -p raylib/win && cd raylib/src && make PLATFORM=PLATFORM_DESKTOP OS=Windows_NT CC=x86_64-w64-mingw32-gcc AR=x86_64-w64-mingw32-ar RAYLIB_RELEASE_PATH=../win && make clean)
python3 tools/embed_fonts.py
x86_64-w64-mingw32-windres res.rc -O coff -o res.o
x86_64-w64-mingw32-gcc -O2 -Iraylib/src main.c res.o raylib/win/libraylib.a -static -lopengl32 -lgdi32 -lwinmm -lm -mwindows -o bin/EngineGarage.exe
