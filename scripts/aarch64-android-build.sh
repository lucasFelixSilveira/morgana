#!/bin/bash

cd compiler

g++ -std=c++17 -g -O0 -fPIC -fpermissive -fexceptions main.cpp \
    -llua \
    -lm \
    -ldl \
    -o morgana

cd ..
