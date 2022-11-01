#!/bin/sh

emcc \
  -o vhacd.js \
  -sALLOW_MEMORY_GROWTH \
  ./vhacd.cpp

