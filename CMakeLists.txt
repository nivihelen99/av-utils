cmake_minimum_required(VERSION 3.10)
project(AsyncValueProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # Important for libraries, especially if they might be static

# --- Library ---
# Since async_value.h is header-only, we create an INTERFACE library.
# This makes it easy for other targets to include the directory.
add_library(async_value INTERFACE)
target_include_directories(async_value INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include> # For installing the library
)

# --- Example ---
add_executable(async_value_example examples/async_value_example.cpp)
target_link_libraries(async_value_example PRIVATE async_value)
# Add C++ standard to executables too if not inherited or for clarity
target_compile_features(async_value_example PRIVATE cxx_std_17)


# --- Tests ---
# Using GTest (FetchContent example)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip # Consider a more specific commit/tag for reproducibility
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(async_value_test tests/async_value_test.cpp)
target_link_libraries(async_value_test PRIVATE async_value GTest::gtest_main)
# Add C++ standard to tests too
target_compile_features(async_value_test PRIVATE cxx_std_17)

include(GoogleTest)
gtest_discover_tests(async_value_test)

# --- Optional: Installation ---
# Example of how you might install the header file
# install(FILES include/async_value.h DESTINATION include)
# install(TARGETS async_value EXPORT AsyncValueTargets)
# install(EXPORT AsyncValueTargets
#   FILE AsyncValueTargets.cmake
#   NAMESPACE AsyncValue::
#   DESTINATION lib/cmake/AsyncValue
# )
