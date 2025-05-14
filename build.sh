#!/bin/sh

# Build raylib library
git clone --depth 1 https://github.com/raysan5/raylib.git raylib
cd raylib/src/
make PLATFORM=PLATFORM_DESKTOP # To make the static version.
sudo make install
cd ../..

rm -rf bin
mkdir bin

gcc main.c \
    player.c \
    terrain.c \
    sky.c \
    lights.c \
    -lraylib -lGL -lm \
    -o bin/rekt
