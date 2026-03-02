#!/bin/bash

# Caminhos
SRC_DIR="lib"
OUT_DIR="lib/bin"
OUT_BIN="$OUT_DIR/morgana"

mkdir -p "$OUT_DIR"

echo "[🔧] Compiling project..."
g++ \
  -std=c++20 \
  -g -O0 \
  -fPIC -fpermissive -fexceptions \
  "$SRC_DIR"/use.cpp "$SRC_DIR"/morgana/*.cpp \
  -I"$SRC_DIR" \
  -o "$OUT_BIN"

echo "[✅] Finished: $OUT_BIN"
echo "[🔨] Running build..."

"./$OUT_BIN"
