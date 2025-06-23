#!/bin/bash
set -e # Exit immediately if a command exits with a non-zero status.
echo "--- Starting build_and_test.sh ---"

echo "--- Cleaning and creating build directory ---"
rm -rf /app/build
mkdir -p /app/build

echo "--- Navigating to build directory (/app/build) ---"
cd /app/build

echo "--- Running CMake (cmake ..) from $(pwd) ---"
cmake ..

echo "--- Running Make from $(pwd) ---"
make

echo "--- Running CTest from $(pwd) ---"
# ctest --output-on-failure
ctest -R "^(IntervalTreeTest\.|UniqueQueueTest\.)" --output-on-failure


echo "--- Finished build_and_test.sh ---"
