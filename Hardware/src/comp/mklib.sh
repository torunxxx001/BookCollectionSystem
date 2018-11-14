#!/bin/bash

pushd ./obj
echo "libs start compile"

g++ ./../lib/xml_lib.cpp ./../lib/barcode.cpp -lpthread -lxerces-c -std=c++0x -I./../lib/ -c

g++ ./../lib/mysqli.cpp $(mysql_config --libs) -std=c++0x -I./../lib/ -c

g++ ./../lib/bigcalc.cpp ./../lib/code128.cpp ./../lib/login_control.cpp -std=c++0x -I./../lib/ -c

g++ ./../lib/lib.cpp ./../lib/base64.cpp ./../lib/isbn.cpp ./../lib/book_manager.cpp -std=c++0x -I./../lib/ -c

echo "libs end compile"
popd
