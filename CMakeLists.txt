cmake_minimum_required(VERSION 3.10)
project(DataStructuresExamples)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")

# Add include directory for header files (globally for all targets in this directory and subdirectories)
include_directories(include)

# Enable testing (should be global)
enable_testing()

# Define executables for examples
add_executable(trie_example examples/example.cpp)
add_executable(policy_example examples/use_policy.cpp)
add_executable(skip_example examples/use_skip.cpp)
add_executable(tcam_example examples/use_tcam.cpp)

# Add the tests subdirectory
# This will process the tests/CMakeLists.txt
add_subdirectory(tests)

# Print a message to confirm CMake configuration
message(STATUS "CMake configuration complete. Examples defined. Tests are in the 'tests' subdirectory.")
