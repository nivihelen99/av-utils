cmake_minimum_required(VERSION 3.10)
project(TaggedUnionProject CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF) # Prefer not to use compiler-specific extensions

# Define the library (header-only)
# For header-only libraries, INTERFACE libraries are suitable.
# They don't compile sources but provide include directories and other properties.
add_library(tagged_union_lib INTERFACE)
target_include_directories(tagged_union_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# --- Test Executable ---
add_executable(tagged_union_tests tests/tagged_union_test.cpp)
target_link_libraries(tagged_union_tests PRIVATE tagged_union_lib)

# Enable testing with CTest (optional, but good practice)
enable_testing()
add_test(NAME TaggedUnionCoreTests COMMAND tagged_union_tests)

# --- Example Executable ---
add_executable(tagged_union_example examples/tagged_union_example.cpp)
target_link_libraries(tagged_union_example PRIVATE tagged_union_lib)

# --- Optional: Installation (if this were to be a distributable library) ---
# install(TARGETS tagged_union_example DESTINATION bin)
# install(FILES include/tagged_union.h DESTINATION include)
# install(TARGETS tagged_union_lib EXPORT TaggedUnionTargets)
# install(EXPORT TaggedUnionTargets
#         FILE TaggedUnionConfig.cmake
#         NAMESPACE TaggedUnion::
#         DESTINATION lib/cmake/TaggedUnion)

# --- Developer options (e.g. warnings) ---
if(MSVC)
    # Add warning options for MSVC
    # target_compile_options(tagged_union_lib INTERFACE /W4) # Example
else()
    # Add warning options for GCC/Clang
    # target_compile_options(tagged_union_lib INTERFACE -Wall -Wextra -pedantic) # Example
endif()
