#!/bin/bash
clang -g -o a.out main.c `pkg-config --cflags --libs libavutil libavformat libavcodec libavutil libswscale libswresample sdl2`