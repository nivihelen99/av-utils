#include <iostream>
#include <string>
#include <vector>
#include "SkipList.h" // Assuming SkipList.h is in the include path configured by CMake

// Helper function to print the status of the skip list
template <typename T, typename Compare, int MaxLevel, double P>
void print_skip_list_status(const cpp_utils::SkipList<T, Compare, MaxLevel, P>& sl, const std::string& name = "SkipList") {
    std::cout << "---- " << name << " Status ----" << std::endl;
    std::cout << "Size: " << sl.size() << std::endl;
    std::cout << "Empty: " << (sl.empty() ? "yes" : "no") << std::endl;
    std::cout << "Current Max Level: " << sl.currentListLevel() << std::endl;
    // Note: To print elements, we would need iterators or a to_vector() method.
    // For this example, we'll rely on contains() to show elements.
    std::cout << "-------------------------" << std::endl;
}

// Example for Point struct and custom comparator
struct Point {
    int x, y;
    bool operator<(const Point& other) const { // Default comparison
        if (x != other.x) return x < other.x;
        return y < other.y;
    }
     bool operator==(const Point& other) const { // For easy checking in example
        return x == other.x && y == other.y;
    }
};

// Custom comparator for Point, comparing by y then x
struct ComparePointYX {
    bool operator()(const Point& a, const Point& b) const {
        if (a.y != b.y) return a.y < b.y;
        return a.x < b.x;
    }
};

// For printing Point objects
std::ostream& operator<<(std::ostream& os, const Point& p) {
    return os << "(" << p.x << ", " << p.y << ")";
}


int main() {
    std::cout << "=== SkipList Example ===" << std::endl;

    // --- Example 1: SkipList with integers (default comparator std::less<int>) ---
    std::cout << "\n--- Integer SkipList Example ---" << std::endl;
    cpp_utils::SkipList<int> int_sl;
    print_skip_list_status(int_sl, "Initial Integer SkipList");

    std::cout << "Inserting 10: " << (int_sl.insert(10) ? "success" : "failed (duplicate)") << std::endl;
    std::cout << "Inserting 5: " << (int_sl.insert(5) ? "success" : "failed (duplicate)") << std::endl;
    std::cout << "Inserting 20: " << (int_sl.insert(20) ? "success" : "failed (duplicate)") << std::endl;
    std::cout << "Inserting 10 again: " << (int_sl.insert(10) ? "success" : "failed (duplicate)") << std::endl;
    print_skip_list_status(int_sl, "After insertions");

    std::cout << "Contains 10? " << (int_sl.contains(10) ? "yes" : "no") << std::endl;
    std::cout << "Contains 15? " << (int_sl.contains(15) ? "yes" : "no") << std::endl;

    std::cout << "Erasing 5: " << (int_sl.erase(5) ? "success" : "failed (not found)") << std::endl;
    print_skip_list_status(int_sl, "After erasing 5");
    std::cout << "Contains 5? " << (int_sl.contains(5) ? "yes" : "no") << std::endl;

    std::cout << "Erasing 100 (non-existent): " << (int_sl.erase(100) ? "success" : "failed (not found)") << std::endl;
    print_skip_list_status(int_sl, "After trying to erase 100");

    int_sl.clear();
    print_skip_list_status(int_sl, "After clear");

    // --- Example 2: SkipList with strings and custom parameters ---
    std::cout << "\n--- String SkipList Example (MaxLevel=8, P=0.25) ---" << std::endl;
    cpp_utils::SkipList<std::string, std::less<std::string>, 8, 0.25> string_sl;

    string_sl.insert("banana");
    string_sl.insert("apple");
    string_sl.insert("orange");
    string_sl.insert("grape");
    print_skip_list_status(string_sl, "String SkipList");

    std::cout << "Contains 'apple'? " << (string_sl.contains("apple") ? "yes" : "no") << std::endl;
    std::cout << "Contains 'mango'? " << (string_sl.contains("mango") ? "yes" : "no") << std::endl;

    // --- Example 3: SkipList with custom type and comparator ---
    std::cout << "\n--- Custom Type (Point) SkipList Example ---" << std::endl;
    cpp_utils::SkipList<Point, ComparePointYX> point_sl;
    Point p1 = {10, 20};
    Point p2 = {5, 30};  // ComparePointYX: (y then x) -> p2 comes after p1
    Point p3 = {15, 20}; // ComparePointYX: (y then x) -> p3 comes after p1 (same y, larger x)
    Point p4 = {5, 10};  // ComparePointYX: (y then x) -> p4 comes before p1, p2, p3

    point_sl.insert(p1);
    point_sl.insert(p2);
    point_sl.insert(p3);
    point_sl.insert(p4);
    print_skip_list_status(point_sl, "Point SkipList (ComparePointYX)");

    std::cout << "Contains (10,20)? " << (point_sl.contains({10,20}) ? "yes" : "no") << std::endl;
    std::cout << "Contains (5,30)? " << (point_sl.contains({5,30}) ? "yes" : "no") << std::endl;
    std::cout << "Contains (5,10)? " << (point_sl.contains({5,10}) ? "yes" : "no") << std::endl;

    std::cout << "Erasing (5,30): " << (point_sl.erase({5,30}) ? "success" : "failed") << std::endl;
    std::cout << "Contains (5,30) after erase? " << (point_sl.contains({5,30}) ? "yes" : "no") << std::endl;
    print_skip_list_status(point_sl, "After erasing (5,30)");

    std::cout << "\n=== Example End ===" << std::endl;
    return 0;
}
