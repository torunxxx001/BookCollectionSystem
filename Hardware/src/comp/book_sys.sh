#!/bin/bash

echo "book_sys start compile"

g++ ./obj/memory.o ./obj/rpi_gpio.o ./obj/spi_dma.o ./obj/board_io.o ./obj/display.o ./obj/scr_obj.o ./obj/img_lib.o ./obj/disp_lib.o ./obj/dstring.o ./obj/xml_lib.o ./obj/barcode.o ./obj/mysqli.o ./obj/login_control.o ./obj/bigcalc.o ./obj/code128.o ./obj/base64.o ./obj/lib.o ./obj/isbn.o ./obj/book_manager.o ./obj/framebuffer.o   ./ex_display.cpp ./ex_scr_obj.cpp ./fb_scr_obj.cpp ./events.cpp -lgd -lpthread -lxerces-c -std=c++0x book_sys.cpp $(mysql_config --libs) -I./hard/ -I./screen/ -I./lib/ -I./ -o book_sys

echo "book_sys end compile"





cp ./book_sys ../data/book_sys
chmod 777 ../data/book_sys
