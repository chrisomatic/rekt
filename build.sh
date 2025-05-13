#!/bin/sh

rm -rf bin
mkdir bin

gcc main.c \
    player.c \
    terrain.c \
    sky.c \
    -lraylib -lGL -lm \
    -o bin/rekt
