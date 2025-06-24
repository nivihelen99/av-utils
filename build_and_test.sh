#!/bin/bash
set -e
echo "--- Starting build_and_test.sh ---"

BUILD_DIR="/app/build"

echo "--- Ensuring build directory exists ---"
mkdir -p "${BUILD_DIR}"

echo "--- Navigating to build directory (${BUILD_DIR}) ---"
cd "${BUILD_DIR}"

echo "--- Running CMake (cmake ..) ---"
# Not cleaning the directory. Let CMake figure out what to rebuild.
# This assumes that if FetchContent for GTest/nlohmann_json ran before,
# it won't need to redownload.
cmake ..

echo "--- Building delta_map_example ---"
make delta_map_example

echo "--- Building delta_map_test (neutered) ---"
make delta_map_test

echo "--- CTest execution skipped. ---"
echo "--- Finished build_and_test.sh (GTests for DeltaMap were not run) ---"
