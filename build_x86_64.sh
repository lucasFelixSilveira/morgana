#!/bin/bash

cd compiler

g++ -std=c++17 -g -O0 -fPIC -fpermissive -fexceptions -llzma -lm main.cpp libs/linux/x86_64/libruna.a \
    -o ../bin/morgana \
    -I. \
    -lstdc++
cd ..
