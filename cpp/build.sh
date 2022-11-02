#!/bin/sh

emcc \
  -lembind \
  -o vhacd.js \
  -sALLOW_MEMORY_GROWTH \
  -sSINGLE_FILE=1 \
  ./vhacd.cpp

