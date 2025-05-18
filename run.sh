#!/bin/bash

clang main.c -o main
if [ $? -eq 0 ]; then
    ./main
else
    echo "Compilation failed with clang"
    echo "Attempting with gcc..."
    gcc main.c -o main
    if [ $? -eq 0 ]; then
        ./main
    else
        echo "Compilation failed with gcc as well"
    fi
fi
