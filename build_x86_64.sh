#!/bin/bash

cd compiler

g++ -std=c++17 -g -O0 -fPIC -fpermissive -fexceptions -llzma -lm main.cpp libs/linux/x86_64/libruna.a \
    -L./libs/linux/x86_64/ -leva \
    -o ../bin/morgana \
    -I. \
    -lstdc++ \
    -Wl,-rpath,'$ORIGIN'

cp libs/linux/x86_64/libeva.so ../bin/

cd ..
