import re

def update_readme():
    try:
        with open("README.md", "r", encoding="utf-8") as f:
            current_readme_content = f.read()
    except FileNotFoundError:
        print("Error: README.md not found.")
        return

    new_dir_structure_section = """## Directory Structure

The project is organized as follows:

- \`CMakeLists.txt\`: The main CMake file to configure and build the project.
- \`include/\`: Contains the header files for the data structures (\`skiplist.h\`, \`trie.h\`, \`policy_radix.h\`). Since these are header-only libraries, you primarily interact with these files.
- \`examples/\`: Contains example source files (\`example.cpp\`, \`use_policy.cpp\`, \`use_skip.cpp\`) demonstrating how to use the data structures.
- \`tests/\`: Contains test source files and related data.
  - \`tests/CMakeLists.txt\`: CMake file specifically for building and running tests using Google Test.
  - \`tests/trie_test.txt\`: Example test data for the Trie.
- \`.gitignore\`: Specifies intentionally untracked files that Git should ignore.
"""

    new_compile_section = """## How to Compile & Run

This project uses CMake to manage the build process. The data structures themselves are header-only and located in the \`include/\` directory. Examples and tests are provided to demonstrate usage and verify functionality.

### Prerequisites
- A C++17 compliant compiler (e.g., GCC, Clang, MSVC)
- CMake (version 3.10 or higher recommended)

### Building the Project (Examples and Tests)

1.  **Clone the repository:**
    \`\`\`bash
    git clone <repository_url>
    cd <repository_directory>
    \`\`\`

2.  **Configure with CMake:**
    It's recommended to build in a separate directory (e.g., \`build/\`):
    \`\`\`bash
    mkdir build
    cd build
    cmake ..
    \`\`\`
    This will configure the project and generate build files for your environment (e.g., Makefiles on Linux/macOS, Visual Studio solution on Windows).

3.  **Compile:**
    \`\`\`bash
    cmake --build .
    \`\`\`
    Or, if using Makefiles (after \`cmake ..\`):
    \`\`\`bash
    make
    \`\`\`
    This will compile the example executables (e.g., \`trie_example\`, \`policy_example\`, \`skip_example\`) and the test runner (\`run_tests\`). The executables will typically be found in the \`build/\` directory.

### Running Examples

After successful compilation, you can run the examples from the build directory:
\`\`\`bash
./trie_example
./policy_example
./skip_example
\`\`\`
*(Note: On Windows, they would be \`.exe\` files, e.g., \`./trie_example.exe\`)*

### Running Tests

The tests are compiled into an executable named \`run_tests\`. You can run it from the build directory:
\`\`\`bash
./run_tests
\`\`\`
Or, using CTest (which is configured by CMake):
\`\`\`bash
ctest
\`\`\`
CTest will provide a summary of test results.
"""

    # Split points
    features_overview_marker = "## Features Overview"
    old_compile_marker = "## How to Compile & Run Examples"
    skip_list_marker = "## Skip List (`skiplist.h`)"

    # Find split points
    try:
        intro_end_index = current_readme_content.index(features_overview_marker)
        old_compile_start_index = current_readme_content.index(old_compile_marker)
        skip_list_start_index = current_readme_content.index(skip_list_marker)

        features_overview_end_index = old_compile_start_index

    except ValueError:
        print("Error: One of the section markers not found in README.md. Cannot proceed.")
        return

    intro_content = current_readme_content[:intro_end_index]
    features_content = current_readme_content[intro_end_index:features_overview_end_index]
    rest_of_readme = current_readme_content[skip_list_start_index:]

    # Assemble the new README
    new_readme_parts = [
        intro_content,
        new_dir_structure_section,
        "\n", # Ensure a blank line
        features_content,
        # The old compile section is skipped here
        new_compile_section,
        "\n", # Ensure a blank line
        rest_of_readme
    ]

    final_readme_content = "".join(new_readme_parts)

    try:
        with open("README.md", "w", encoding="utf-8") as f:
            f.write(final_readme_content)
        print("README.md updated successfully.")
    except IOError:
        print("Error: Could not write updated README.md.")

if __name__ == "__main__":
    update_readme()
