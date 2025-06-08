cmake_minimum_required(VERSION 3.10)
project(DataStructuresExamples)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")

# Add include directory for header files (globally for all targets in this directory and subdirectories)
include_directories(include) # Prefer target_include_directories

# --- Variant Vector Library (Header-Only) ---
add_library(variant_vector INTERFACE)
add_library(cpp_library::variant_vector ALIAS variant_vector) # Modern CMake alias
target_include_directories(variant_vector INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
# This makes sure anyone linking against variant_vector gets 'include' in their include path.

# Enable testing (should be global)
enable_testing()

# Define executables for examples
add_executable(trie_example examples/example.cpp)
add_executable(policy_example examples/use_policy.cpp)
add_executable(skip_example examples/use_skip.cpp)
add_executable(tcam_example examples/use_tcam.cpp)
add_executable(arp_cache_example examples/arp_cache_example.cpp)
add_executable(nd_cache_example examples/nd_cache_example.cpp)
add_executable(use_dsu_example examples/use_dsu.cpp)
add_executable(use_persist_array_example examples/use_persist_array.cpp)

# Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Add the tests subdirectory
# This will process the tests/CMakeLists.txt
add_subdirectory(tests)

# Print a message to confirm CMake configuration
message(STATUS "CMake configuration complete. Examples defined. Tests are in the 'tests' subdirectory.")
