#include "history.h"
#include <iostream>
#include <vector>
#include <string>

void print_vector(const std::vector<int>& vec) {
    std::cout << "{ ";
    for (const auto& i : vec) {
        std::cout << i << " ";
    }
    std::cout << "}" << std::endl;
}

int main() {
    // Create a history for a vector of integers
    cpp_collections::History<std::vector<int>> history({1, 2, 3});

    std::cout << "Initial state (v0): ";
    print_vector(history.latest());
    std::cout << "Total versions: " << history.versions() << std::endl;
    std::cout << "-------------------------" << std::endl;

    // Modify the vector and commit the change
    history.latest().push_back(4);
    history.commit();
    std::cout << "After adding 4 (v1): ";
    print_vector(history.latest());
    std::cout << "Total versions: " << history.versions() << std::endl;
    std::cout << "-------------------------" << std::endl;

    // Modify again and commit
    history.latest().pop_back();
    history.latest().push_back(5);
    history.commit();
    std::cout << "After replacing 4 with 5 (v2): ";
    print_vector(history.latest());
    std::cout << "Total versions: " << history.versions() << std::endl;
    std::cout << "-------------------------" << std::endl;

    // Access previous versions
    std::cout << "Version 0: ";
    print_vector(history.get(0));
    std::cout << "Version 1: ";
    print_vector(history.get(1));
    std::cout << "Version 2: ";
    print_vector(history.get(2));
    std::cout << "-------------------------" << std::endl;

    // Revert to a previous version
    history.revert(1);
    std::cout << "After reverting to v1 (new v3): ";
    print_vector(history.latest());
    std::cout << "Total versions: " << history.versions() << std::endl;
    std::cout << "-------------------------" << std::endl;

    // Show that the reverted state is a new version
    std::cout << "Version 3: ";
    print_vector(history.get(3));
    std::cout << "Latest version is now: " << history.current_version() << std::endl;

    return 0;
}
