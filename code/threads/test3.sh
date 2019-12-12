#!/bin/bash
cd ../build.linux
echo "Rebuild NachOS"
make clean
make

cd ../test
make clean
make
../build.linux/nachos -ep hw3t1 90 -ep hw3t2 100 -d z #L2L1
echo "nachos -ep hw3t1 90 -ep hw3t2 100 #L2L1"
