#!/bin/bash

pushd ./obj
echo "hard start compile"

g++ ./../hard/memory.c ./../hard/rpi_gpio.cpp ./../hard/spi_dma.cpp ./../hard/board_io.cpp -std=c++0x -I./../hard/ -I./../ -c

echo "hard end compile"
popd
