#!/bin/bash

set -eu

if [ $# -ne 1 ]; then
  echo "Error: there must be only 1 argument" >&2
fi

g++ -o OSLab2 OSLab2.cpp -std=c++17

filename=$(basename "$1" .cpp)
./"$filename"
