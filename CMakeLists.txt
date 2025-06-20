cmake_minimum_required(VERSION 3.10)
project(HeapQueueProject CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Define HeapQueue as an interface library (header-only)
add_library(HeapQueue INTERFACE)
target_include_directories(HeapQueue INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Future steps will add examples and tests here

# Add example executable
add_executable(SimpleUsageExample examples/simple_usage.cpp)

# Link example against HeapQueue
target_link_libraries(SimpleUsageExample PRIVATE HeapQueue)

# Optional: Print a message where the executable can be found
# Get the directory where the executable will be built
if(CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(EXE_OUTPUT_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
else()
    set(EXE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
endif()

message(STATUS "SimpleUsageExample executable will be built in: ${EXE_OUTPUT_PATH}/")

# Google Test setup
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip # Or a specific commit/tag
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# Enable testing for the project
enable_testing()

message(STATUS "Google Test setup complete. Tests will be added next.")
