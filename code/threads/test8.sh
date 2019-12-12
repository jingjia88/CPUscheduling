#!/bin/bash
cd ../build.linux
echo "Rebuild NachOS"
make clean
make

cd ../test
make clean
make
../build.linux/nachos -ep hw3t1 0 -ep hw3t2 0 -d z #L3
echo "nachos -ep hw3t1 0 -ep hw3t2 0 #L3"
