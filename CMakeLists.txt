cmake_minimum_required(VERSION 3.10)
project(DataStructuresExamples)

find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 20)
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

# --- Counter Library (Header-Only) ---
add_library(Counter INTERFACE)
add_library(cpp_library::Counter ALIAS Counter) # Modern CMake alias
target_include_directories(Counter INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# Enable testing (should be global)
enable_testing()

# Automatically define executables for examples
file(GLOB EXAMPLE_SOURCES "examples/*.cpp")
foreach(SOURCE_FILE ${EXAMPLE_SOURCES})
    if(NOT "${SOURCE_FILE}" STREQUAL "examples/use_spsc.cpp")
        get_filename_component(BASE_NAME ${SOURCE_FILE} NAME_WE)
        set(EXECUTABLE_NAME "example_${BASE_NAME}")
        add_executable(${EXECUTABLE_NAME} ${SOURCE_FILE})
        # Link common libraries if needed by examples
        # Added Threads::Threads for examples that might use threading features from components like LRUCache
        target_link_libraries(${EXECUTABLE_NAME} PRIVATE cpp_library::variant_vector cpp_library::Counter Threads::Threads)
    endif()
endforeach()

# --- SPSC Queue Example ---
# This example already links Threads::Threads, so the change above is consistent.
add_executable(use_spsc_example examples/use_spsc.cpp)
target_include_directories(use_spsc_example PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_link_libraries(use_spsc_example PRIVATE Threads::Threads)


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
