#!/bin/bash

cd compiler

g++ -std=c++17 -g -O0 -fPIC -fpermissive -fexceptions -llzma -lm main.cpp \
    -L./libs/x86_64-linux -leva -lruna \
    -o ../bin/morgana \
    -I. \
    -lstdc++ \
    -Wl,-rpath,'$ORIGIN'

cp libs/linux/x86_64/libeva.so ../bin/
cp libs/linux/x86_64/libruna.so ../bin/

cd ..
