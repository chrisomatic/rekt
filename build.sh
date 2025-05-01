#!/bin/sh

rm -rf bin
mkdir bin

gcc main.c \
    player.c \
    -lraylib -lGL -lm \
    -o bin/rekt
