#!/bin/bash

pushd ./obj
echo "scr start compile"

g++ ./../screen/display.cpp ./../screen/scr_obj.cpp ./../screen/img_lib.cpp ./../screen/disp_lib.cpp ./../screen/dstring.cpp -lgd -lpthread -std=c++0x -I./../hard/ -I./../screen/ -I./../ -c

echo "scr end compile"
popd
