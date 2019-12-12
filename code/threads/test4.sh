#!/bin/bash
cd ../build.linux
echo "Rebuild NachOS"
make clean
make

cd ../test
make clean
make
../build.linux/nachos -ep hw3t1 40 -ep hw3t2 90 -d z #L3L2
echo "nachos -ep hw3t1 40 -ep hw3t2 90 #L3L2"
