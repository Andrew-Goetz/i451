#!/bin/sh
export LD_LIBRARY_PATH=../../../cs551/lib/
gcc -I ../../../cs551/include/ knnd.c -L ../../../cs551/lib/ -lknn_ocr -o knnd
gcc -I ../../../cs551/include/ knnc.c -L ../../../cs551/lib/ -lknn_ocr -o knnc
