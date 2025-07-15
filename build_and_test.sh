#!/bin/bash
set -ex
echo "--- Starting build_and_test.sh ---"

BUILD_DIR="/app/build"

echo "--- Cleaning and ensuring build directory exists ---"
if [ -d "${BUILD_DIR}" ]; then
    echo "Removing existing build directory: ${BUILD_DIR}"
    rm -rf "${BUILD_DIR}"
fi
mkdir -p "${BUILD_DIR}"

echo "--- Navigating to build directory (${BUILD_DIR}) ---"
cd "${BUILD_DIR}"

echo "--- Running CMake (cmake ..) ---"
cmake ..

echo "--- Building all targets ---"
make -j2 # Build all targets with reduced parallelism

echo "--- Running TrieMap_test ---"
./tests/TrieMap_test

echo "--- Finished build_and_test.sh ---"
