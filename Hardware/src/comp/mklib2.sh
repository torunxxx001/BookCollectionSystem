#!/bin/bash

pushd ./obj
echo "libs start compile"

g++ ./../lib/framebuffer.cpp -std=c++0x -I./../ -I./../lib/ -lgd -c

echo "libs end compile"
popd
