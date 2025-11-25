#!/bin/bash

set -eu

if [ $# -ne 1 ]; then
  echo "Error: there must be only 1 argument" >&2
fi

g++ -o program program.cpp -lssl -lcrypto -std=c++17

mkdir -p testdir
echo "test1" > testdir/test1.txt
echo "test2" > testdir/test2.txt
echo "Lab3" > testdir/test3.txt
echo "Lab3" > testdir/test4.txt

filename=$(basename "$1" .cpp)
./"$filename"
 
