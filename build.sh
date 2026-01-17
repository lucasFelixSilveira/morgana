#!/bin/bash

cd compiler

g++ -std=c++17 -g -O0 -fPIC -fpermissive -fexceptions main.cpp libs/linux/liblua54.a \
    -o ../bin/morgana \
    -I.
cd ..
