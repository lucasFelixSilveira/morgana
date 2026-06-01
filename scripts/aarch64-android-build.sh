#!/bin/bash

cd compiler

clang++ -std=c++17 -g -O0 -fPIC -fpermissive -fexceptions -llzma -lm main.cpp \
    -L./libs/aarch64-android -leva -lruna \
    -o ../bin/morgana \
    -I. \
    -lstdc++ \
    -Wl,-rpath,'$ORIGIN'

cd ..
