#!/bin/bash

rm -rf Xcode && mkdir Xcode && cd Xcode
cmake -G Xcode ..

echo "========================================="
echo "build xcode success"
echo "========================================="
