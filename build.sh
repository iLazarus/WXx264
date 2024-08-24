#!/bin/bash

CURR_BUILD_DIR=$(pwd)

rm -rf $CURR_BUILD_DIR/lib $CURR_BUILD_DIR/include $CURR_BUILD_DIR/build/*.js $CURR_BUILD_DIR/build/*.wasm

cd $CURR_BUILD_DIR/x264

ARGS=(
  --prefix=$CURR_BUILD_DIR
  --disable-cli
  --enable-static
  --disable-bashcompletion
  --disable-opencl
  --disable-thread
  --disable-win32thread
  --bit-depth=8
  --chroma-format=420
  --disable-asm
  --enable-lto
  --enable-strip
  --enable-pic
  --disable-avs
  --disable-lavf
  --disable-ffms
  --disable-gpac
  --disable-lsmash
  --host=i686-linux-gnu
)
emmake make distclean
emconfigure ./configure "${ARGS[@]}"
emmake make -j4
emmake make install

cd $CURR_BUILD_DIR

emcc \
-O3 \
-Iinclude \
-Llib \
-lx264 \
-lm \
-sWASM=1 \
-sALLOW_TABLE_GROWTH \
-sEXPORTED_FUNCTIONS='["_create", "_encoder", "_destroy", "_malloc", "_free"]' \
src/h264.c \
-o $CURR_BUILD_DIR/build/h264.js