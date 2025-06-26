#include "deque.h" // Adjust path if necessary based on your build system
#include <iostream>
#include <string>
#include <vector>

int main() {
    // Create an empty Deque of integers
    ankerl::Deque<int> d1;
    std::cout << "Created an empty deque d1." << std::endl;

    // Add elements to the back
    std::cout << "\nPushing elements to the back of d1:" << std::endl;
    d1.push_back(10);
    d1.push_back(20);
    d1.push_back(30); // d1: [10, 20, 30]
    std::cout << "d1: ";
    for (int val : d1) {
        std::cout << val << " ";
    }
    std::cout << std::endl; // Expected: 10 20 30

    // Add elements to the front
    std::cout << "\nPushing elements to the front of d1:" << std::endl;
    d1.push_front(5);
    d1.push_front(0); // d1: [0, 5, 10, 20, 30]
    std::cout << "d1: ";
    for (size_t i = 0; i < d1.size(); ++i) {
        std::cout << d1[i] << " ";
    }
    std::cout << std::endl; // Expected: 0 5 10 20 30

    // Access elements
    std::cout << "\nAccessing elements:" << std::endl;
    std::cout << "d1.front(): " << d1.front() << std::endl; // Expected: 0
    std::cout << "d1.back(): " << d1.back() << std::endl;   // Expected: 30
    std::cout << "d1[2]: " << d1[2] << std::endl;           // Expected: 10
    try {
        std::cout << "d1.at(1): " << d1.at(1) << std::endl; // Expected: 5
        std::cout << "d1.at(10): "; // This will throw
        std::cout << d1.at(10) << std::endl;
    } catch (const std::out_of_range& e) {
        std::cout << "Exception caught: " << e.what() << std::endl; // Expected
    }

    // Remove elements
    std::cout << "\nRemoving elements:" << std::endl;
    d1.pop_front(); // d1: [5, 10, 20, 30]
    std::cout << "After pop_front(), d1.front(): " << d1.front() << std::endl; // Expected: 5
    d1.pop_back();  // d1: [5, 10, 20]
    std::cout << "After pop_back(), d1.back(): " << d1.back() << std::endl;   // Expected: 20

    std::cout << "Current d1: ";
    for (const auto& val : d1) {
        std::cout << val << " ";
    }
    std::cout << std::endl; // Expected: 5 10 20

    // Size and empty check
    std::cout << "\nSize and empty check:" << std::endl;
    std::cout << "d1.size(): " << d1.size() << std::endl;     // Expected: 3
    std::cout << "d1.empty(): " << (d1.empty() ? "true" : "false") << std::endl; // Expected: false

    // Clear the deque
    std::cout << "\nClearing d1:" << std::endl;
    d1.clear();
    std::cout << "d1.size() after clear: " << d1.size() << std::endl; // Expected: 0
    std::cout << "d1.empty() after clear: " << (d1.empty() ? "true" : "false") << std::endl; // Expected: true

    // Constructor with initializer list
    std::cout << "\nDeque d2 initialized with { \"apple\", \"banana\", \"cherry\" }:" << std::endl;
    ankerl::Deque<std::string> d2 = { "apple", "banana", "cherry" };
    std::cout << "d2: ";
    for (const std::string& s : d2) {
        std::cout << "\"" << s << "\" ";
    }
    std::cout << std::endl;

    // Using iterators explicitly
    std::cout << "\nIterating d2 using iterators:" << std::endl;
    for (ankerl::Deque<std::string>::iterator it = d2.begin(); it != d2.end(); ++it) {
        std::cout << *it << " ";
        if (*it == "banana") {
            // Note: Modifying elements via non-const iterator would be possible here if needed
            // *it = "blueberry";
        }
    }
    std::cout << std::endl;

    // Copy constructor and assignment
    std::cout << "\nCopying deques:" << std::endl;
    ankerl::Deque<std::string> d3 = d2; // Copy constructor
    std::cout << "d3 (copy of d2): ";
    for (const auto& s : d3) std::cout << s << " ";
    std::cout << std::endl;

    ankerl::Deque<std::string> d4;
    d4.push_back("old_value");
    d4 = d2; // Copy assignment
    std::cout << "d4 (assigned from d2): ";
    for (const auto& s : d4) std::cout << s << " ";
    std::cout << std::endl;

    std::cout << "\nEnd of Deque example." << std::endl;

    return 0;
}
