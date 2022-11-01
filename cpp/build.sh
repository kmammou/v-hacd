#!/bin/sh

emcc \
  -lembind \
  -o vhacd.js \
  -sALLOW_MEMORY_GROWTH \
  ./vhacd.cpp

