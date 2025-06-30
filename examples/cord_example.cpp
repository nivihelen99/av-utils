#include "cord.h"
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>

void print_cord_details(const std::string& name, const Cord& cord) {
    std::cout << "Cord \"" << name << "\":\n";
    std::cout << "  Length: " << cord.length() << "\n";
    std::cout << "  Empty: " << (cord.empty() ? "true" : "false") << "\n";
    try {
        std::cout << "  ToString(): \"" << cord.ToString() << "\"\n";
    } catch (const std::exception& e) {
        std::cout << "  ToString() failed: " << e.what() << "\n";
    }
    std::cout << "  First char (if not empty): ";
    if (!cord.empty()) {
        try {
            std::cout << "'" << cord[0] << "' (via operator[]) | '" << cord.at(0) << "' (via at())\n";
        } catch (const std::out_of_range& e) {
            std::cout << "Error accessing first char: " << e.what() << "\n";
        }
    } else {
        std::cout << "N/A (empty)\n";
    }
    std::cout << "-----\n";
}

int main() {
    std::cout << "--- Cord Example ---" << std::endl;

    // 1. Cord creation
    Cord c1; // Default constructor
    print_cord_details("c1 (default)", c1);

    Cord c2("Hello"); // From const char*
    print_cord_details("c2 (\"Hello\")", c2);

    std::string str_val = ", World";
    Cord c3(str_val); // From std::string lvalue
    print_cord_details("c3 (from std::string lvalue)", c3);

    Cord c4(std::string("!")); // From std::string rvalue
    print_cord_details("c4 (from std::string rvalue)", c4);

    Cord c5 = c2; // Copy constructor
    print_cord_details("c5 (copy of c2)", c5);

    Cord c5_move = std::move(c5); // Move constructor
    print_cord_details("c5_move (moved from c5)", c5_move);
    print_cord_details("c5 (after move)", c5); // c5 should be in a valid but unspecified state

    // Assignment
    Cord c_assign;
    c_assign = "Assigned C-string";
    print_cord_details("c_assign (from C-string)", c_assign);
    std::string assign_str = "Assigned std::string";
    c_assign = assign_str;
    print_cord_details("c_assign (from std::string lvalue)", c_assign);
    c_assign = std::string("Assigned std::string rvalue");
    print_cord_details("c_assign (from std::string rvalue)", c_assign);
    Cord c_assign_cord("Cord for assignment");
    c_assign = c_assign_cord; // Copy assignment
    print_cord_details("c_assign (from Cord copy)", c_assign);
    c_assign = std::move(c_assign_cord); // Move assignment
    print_cord_details("c_assign (from Cord move)", c_assign);


    // 2. Concatenation
    Cord c6 = c2 + c3; // Cord + Cord
    print_cord_details("c6 (c2 + c3)", c6);

    Cord c7 = c6 + " How are you?"; // Cord + const char*
    print_cord_details("c7 (c6 + \" How are you?\")", c7);

    std::string suffix_str = " Fine, thanks.";
    Cord c8 = c7 + suffix_str; // Cord + std::string
    print_cord_details("c8 (c7 + std::string)", c8);

    Cord c9 = "Prefix: " + c8; // const char* + Cord
    print_cord_details("c9 (\"Prefix: \" + c8)", c9);

    Cord c10 = std::string("String Prefix: ") + c9; // std::string + Cord
    print_cord_details("c10 (std::string + c9)", c10);

    // 3. Length and Empty (already shown in print_cord_details)
    std::cout << "c10 length: " << c10.length() << std::endl;
    std::cout << "c1 empty? " << std::boolalpha << c1.empty() << std::endl;
    std::cout << "c10 empty? " << std::boolalpha << c10.empty() << std::endl;

    // 4. Character access
    std::cout << "\n--- Character Access on c10 (" << c10.ToString() << ") ---" << std::endl;
    if (!c10.empty()) {
        std::cout << "c10[0]: " << c10[0] << std::endl;
        std::cout << "c10.at(7): " << c10.at(7) << std::endl;
        try {
            std::cout << "c10.at(" << c10.length() << "): ";
            std::cout << c10.at(c10.length()) << std::endl; // Should throw
        } catch (const std::out_of_range& e) {
            std::cout << "Caught expected exception: " << e.what() << std::endl;
        }
    }

    // 5. Substring
    std::cout << "\n--- Substring Examples (from c10) ---" << std::endl;
    Cord sub1 = c10.substr(0, 15); // "String Prefix: "
    print_cord_details("sub1 (c10.substr(0, 15))", sub1);

    Cord sub2 = c10.substr(c10.length() - 14); // " Fine, thanks."
    print_cord_details("sub2 (c10.substr(c10.length() - 14))", sub2);

    Cord sub3 = c10.substr(15, 20); // "Prefix: Hello, Wor"
    print_cord_details("sub3 (c10.substr(15, 20))", sub3);

    Cord sub_full = c10.substr(); // Whole string
    print_cord_details("sub_full (c10.substr())", sub_full);

    Cord sub_empty_end = c10.substr(c10.length());
    print_cord_details("sub_empty_end (c10.substr(c10.length()))", sub_empty_end);

    Cord sub_empty_count0 = c10.substr(5, 0);
    print_cord_details("sub_empty_count0 (c10.substr(5,0))", sub_empty_count0);

    try {
        Cord sub_invalid_pos = c10.substr(c10.length() + 1);
        print_cord_details("sub_invalid_pos (should not print if exception)", sub_invalid_pos);
    } catch (const std::out_of_range& e) {
        std::cout << "Caught expected exception for substr out of bounds: " << e.what() << std::endl;
    }


    // 6. Clear
    std::cout << "\n--- Clear Example ---" << std::endl;
    Cord c_to_clear = "This will be cleared.";
    print_cord_details("c_to_clear (before clear)", c_to_clear);
    c_to_clear.clear();
    print_cord_details("c_to_clear (after clear)", c_to_clear);

    // 7. Test with empty strings in various places
    std::cout << "\n--- Empty String Tests ---" << std::endl;
    Cord e1("");
    Cord e2("data");
    Cord e3 = e1 + e2;
    print_cord_details("e3 (e1 + e2 where e1 is empty)", e3);
    Cord e4 = e2 + e1;
    print_cord_details("e4 (e2 + e1 where e1 is empty)", e4);
    Cord e5 = e1 + e1;
    print_cord_details("e5 (e1 + e1 where e1 is empty)", e5);

    Cord sub_from_empty = e1.substr(0,0);
    print_cord_details("sub_from_empty (e1.substr(0,0))", sub_from_empty);

    std::cout << "\n--- Example End ---" << std::endl;

    return 0;
}
