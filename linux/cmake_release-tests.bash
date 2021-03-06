#!/bin/bash

rm CMakeCache.txt
cmake -DMATH_TESTS_EXECUTABLE=1 -DFAIL_USING_EXCEPTIONS=1 -DLINUX=1 -DCMAKE_BUILD_TYPE=Release $* -G "Unix Makefiles" ..

echo "make --version"
make --version

echo " "

echo "gcc --version"
gcc --version
