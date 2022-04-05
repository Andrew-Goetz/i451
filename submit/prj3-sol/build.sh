#!/bin/sh
make -C knnd-src
mv knnd-src/knnd .
make -C knnc-src
mv knnc-src/knnc .
