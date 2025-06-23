cmake_minimum_required(VERSION 3.10)
project(HeapQueueProject CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Define HeapQueue as an interface library (header-only)
add_library(HeapQueue INTERFACE)
target_include_directories(HeapQueue INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Define other cpp_library components as interface libraries
add_library(CppLibCounter INTERFACE)
target_include_directories(CppLibCounter INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(CppLibLazySortedMerger INTERFACE)
target_include_directories(CppLibLazySortedMerger INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(CppLibAsyncEventQueue INTERFACE)
target_include_directories(CppLibAsyncEventQueue INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(CppLibContextMgr INTERFACE)
target_include_directories(CppLibContextMgr INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(OrderedSet INTERFACE)
target_include_directories(OrderedSet INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(BoundedSetLib INTERFACE)
target_include_directories(BoundedSetLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(SlidingWindowMinMax INTERFACE)
target_include_directories(SlidingWindowMinMax INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(ZipViewLib INTERFACE)
target_include_directories(ZipViewLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(BatcherLib INTERFACE)
target_include_directories(BatcherLib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(ScopedTimer INTERFACE)
target_include_directories(ScopedTimer INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(RetryUtil INTERFACE)
target_include_directories(RetryUtil INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Future steps will add examples and tests here

# Automatically add executables for all files in the examples/ directory
file(GLOB EXAMPLE_FILES "examples/*.cpp")

foreach(EXAMPLE_FILE ${EXAMPLE_FILES})
    # Derive executable name from source file name
    get_filename_component(EXECUTABLE_NAME ${EXAMPLE_FILE} NAME_WE)

    # Add executable target
    add_executable(${EXECUTABLE_NAME} ${EXAMPLE_FILE})

    # Set include directories
    target_include_directories(${EXECUTABLE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

    # Link against relevant libraries
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE
        HeapQueue
        CppLibCounter
        CppLibLazySortedMerger
        CppLibAsyncEventQueue
        CppLibContextMgr
        OrderedSet
        BoundedSetLib
        SlidingWindowMinMax
        ZipViewLib
        BatcherLib
        ScopedTimer
        RetryUtil
    )

    if(EXECUTABLE_NAME STREQUAL "partial_example")
        target_compile_definitions(${EXECUTABLE_NAME} PRIVATE FUNCTOOLS_PARTIAL_EXAMPLES)
    endif()
endforeach()

# Optional: Print a message where the executables can be found
# Get the directory where the executables will be built
if(CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(EXE_OUTPUT_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
else()
    set(EXE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
endif()

message(STATUS "Example executables will be built in: ${EXE_OUTPUT_PATH}/")

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

add_subdirectory(tests)

message(STATUS "Google Test setup complete. Tests will be added next.")
