#include "QuotientFilter.h"
#include <iostream>
#include <string>
#include <vector>

int main() {
    std::cout << "--- QuotientFilter Example ---" << std::endl;

    // Create a Quotient Filter for approximately 1000 items with a 1% false positive rate
    QuotientFilter<int> qf_int(1000, 0.01);
    std::cout << "\nCreated QuotientFilter for integers." << std::endl;
    std::cout << "Initial size: " << qf_int.size() << ", empty: " << (qf_int.empty() ? "yes" : "no") << std::endl;

    // Add some integers
    std::vector<int> my_numbers = {10, 20, 30, 42, 1005};
    std::cout << "\nAdding numbers: ";
    for (int num : my_numbers) {
        qf_int.add(num);
        std::cout << num << " ";
    }
    std::cout << std::endl;
    std::cout << "Size after adds: " << qf_int.size() << std::endl;

    // Check for added numbers
    std::cout << "\nChecking for added numbers:" << std::endl;
    for (int num : my_numbers) {
        std::cout << "might_contain(" << num << "): " << (qf_int.might_contain(num) ? "true" : "false") << std::endl;
    }

    // Check for numbers not added
    std::cout << "\nChecking for numbers NOT added:" << std::endl;
    std::vector<int> other_numbers = {1, 99, 777};
    for (int num : other_numbers) {
        std::cout << "might_contain(" << num << "): " << (qf_int.might_contain(num) ? "true (could be FP)" : "false") << std::endl;
    }

    std::cout << "\n--- QuotientFilter with Strings ---" << std::endl;
    QuotientFilter<std::string> qf_str(100, 0.005);
    std::cout << "Created QuotientFilter for strings." << std::endl;

    std::vector<std::string> my_strings = {"apple", "banana", "cherry", "date"};
    std::cout << "\nAdding strings: ";
    for (const auto& s : my_strings) {
        qf_str.add(s);
        std::cout << "\"" << s << "\" ";
    }
    std::cout << std::endl;
    std::cout << "Size after adds: " << qf_str.size() << std::endl;

    // Check for added strings
    std::cout << "\nChecking for added strings:" << std::endl;
    for (const auto& s : my_strings) {
        std::cout << "might_contain(\"" << s << "\"): " << (qf_str.might_contain(s) ? "true" : "false") << std::endl;
    }

    // Check for strings not added
    std::cout << "\nChecking for strings NOT added:" << std::endl;
    std::vector<std::string> other_strings = {"grape", "kiwi", "applepie"};
    for (const auto& s : other_strings) {
        std::cout << "might_contain(\"" << s << "\"): " << (qf_str.might_contain(s) ? "true (could be FP)" : "false") << std::endl;
    }

    std::cout << "\nExample finished." << std::endl;

    return 0;
}
