#!/bin/bash
cd ../build.linux
echo "Rebuild NachOS"
make clean
make

cd ../test
make clean
make
../build.linux/nachos -ep hw3t1 50 -ep hw3t2 90 -d z #L2
echo "nachos -ep hw3t1 50 -ep hw3t2 90 #L2"
