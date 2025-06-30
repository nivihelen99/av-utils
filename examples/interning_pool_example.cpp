#include "interning_pool.hpp" // Adjust path as necessary
#include <iostream>
#include <string>
#include <vector>

void print_handles_comparison(const std::string& s1_val, const std::string* h1,
                              const std::string& s2_val, const std::string* h2) {
    std::cout << "String 1: \"" << s1_val << "\", Handle 1: " << static_cast<const void*>(h1) << " (Value: \"" << *h1 << "\")\n";
    std::cout << "String 2: \"" << s2_val << "\", Handle 2: " << static_cast<const void*>(h2) << " (Value: \"" << *h2 << "\")\n";
    if (h1 == h2) {
        std::cout << "Handles are THE SAME. The strings are interned to the same object.\n";
    } else {
        std::cout << "Handles are DIFFERENT. The strings are interned to different objects.\n";
    }
    std::cout << "----------------------------------------\n";
}

int main() {
    // Create an InterningPool for std::string
    cpp_collections::InterningPool<std::string> string_pool;

    std::cout << "Initial pool size: " << string_pool.size() << std::endl;
    std::cout << "Pool is empty: " << (string_pool.empty() ? "true" : "false") << std::endl;
    std::cout << "----------------------------------------\n";

    // Intern some strings
    const std::string* handle1 = string_pool.intern("hello");
    const std::string* handle2 = string_pool.intern("world");
    const std::string* handle3 = string_pool.intern("hello"); // Duplicate

    std::cout << "Pool size after interning \"hello\", \"world\", \"hello\": " << string_pool.size() << std::endl;

    // Demonstrate handle equality for identical strings
    std::cout << "Comparing handles for \"hello\" and \"hello\":\n";
    print_handles_comparison("hello", handle1, "hello", handle3);

    // Demonstrate handle inequality for different strings
    std::cout << "Comparing handles for \"hello\" and \"world\":\n";
    print_handles_comparison("hello", handle1, "world", handle2);

    // Intern an empty string
    const std::string* handle_empty1 = string_pool.intern("");
    const std::string* handle_empty2 = string_pool.intern("");
    std::cout << "Pool size after interning two empty strings: " << string_pool.size() << std::endl;
    std::cout << "Comparing handles for empty string and empty string:\n";
    print_handles_comparison("", handle_empty1, "", handle_empty2);

    // Demonstrate interning with rvalue (temporary string)
    const std::string* handle_rvalue = string_pool.intern(std::string("temporary"));
    const std::string* handle_rvalue_dup = string_pool.intern("temporary");
    std::cout << "Pool size after interning rvalue \"temporary\" and lvalue \"temporary\": " << string_pool.size() << std::endl;
    std::cout << "Comparing handles for rvalue \"temporary\" and lvalue \"temporary\":\n";
    print_handles_comparison("temporary", handle_rvalue, "temporary", handle_rvalue_dup);


    // Demonstrate `contains`
    std::cout << "Pool contains \"world\": " << (string_pool.contains("world") ? "true" : "false") << std::endl;
    std::cout << "Pool contains \"new_string\": " << (string_pool.contains("new_string") ? "true" : "false") << std::endl;
    const std::string* handle_new = string_pool.intern("new_string");
    std::cout << "Pool contains \"new_string\" after interning: " << (string_pool.contains("new_string") ? "true" : "false") << std::endl;
    std::cout << "Pool size: " << string_pool.size() << std::endl;
    std::cout << "----------------------------------------\n";

    // Demonstrate `clear`
    std::cout << "Clearing the pool...\n";
    string_pool.clear();
    std::cout << "Pool size after clear: " << string_pool.size() << std::endl;
    std::cout << "Pool is empty after clear: " << (string_pool.empty() ? "true" : "false") << std::endl;

    // Note: After `clear()`, old handles (handle1, handle2, etc.) are invalidated.
    // Accessing them (e.g., *handle1) would be undefined behavior.
    // Re-interning "hello" will likely give a new handle address.
    const std::string* handle_hello_after_clear = string_pool.intern("hello");
    std::cout << "Interning \"hello\" again after clear.\n";
    std::cout << "Old handle1 for \"hello\" (invalid): " << static_cast<const void*>(handle1) << std::endl;
    std::cout << "New handle for \"hello\" after clear: " << static_cast<const void*>(handle_hello_after_clear)
              << " (Value: \"" << *handle_hello_after_clear << "\")\n";
    std::cout << "----------------------------------------\n";


    // Example with a different type (int)
    // Note: Interning basic types like int might be less common but demonstrates template usability.
    // For basic types, the overhead of the pool might outweigh benefits unless there's a
    // very specific scenario (e.g., limited set of int values that are frequently compared by identity).
    cpp_collections::InterningPool<int> int_pool;
    const int* h_int1 = int_pool.intern(100);
    const int* h_int2 = int_pool.intern(200);
    const int* h_int3 = int_pool.intern(100);

    std::cout << "Int pool size: " << int_pool.size() << std::endl; // Expected: 2
    std::cout << "Comparing handles for int 100 and int 100:\n";
    if (h_int1 == h_int3) {
        std::cout << "Handles for 100 are THE SAME. Address: " << static_cast<const void*>(h_int1) << ", Value: " << *h_int1 << std::endl;
    } else {
        std::cout << "Handles for 100 are DIFFERENT.\n";
    }
    std::cout << "Comparing handles for int 100 and int 200:\n";
    if (h_int1 == h_int2) {
        std::cout << "Handles for 100 and 200 are THE SAME.\n";
    } else {
        std::cout << "Handles for 100 and 200 are DIFFERENT. Handle for 200: " << static_cast<const void*>(h_int2) << ", Value: " << *h_int2 << std::endl;
    }
    std::cout << "----------------------------------------\n";


    return 0;
}
