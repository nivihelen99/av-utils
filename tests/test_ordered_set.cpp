#include "ordered_set.h"
#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <list> // For comparing with list operations where appropriate

// Helper to print set contents for debugging
template<typename T, typename H, typename KE>
void print_set(const OrderedSet<T, H, KE>& set, const std::string& name = "") {
    if (!name.empty()) {
        std::cout << name << ": ";
    }
    std::cout << "{ ";
    for (const auto& item : set) {
        std::cout << item << " ";
    }
    std::cout << "}" << std::endl;
}

void test_constructors_and_empty_size() {
    std::cout << "--- Test Constructors, Empty, Size ---" << std::endl;
    OrderedSet<int> s1;
    assert(s1.empty());
    assert(s1.size() == 0);
    // print_set(s1, "s1 (default ctor)");

    OrderedSet<std::string> s2 = {"hello", "world", "hello"};
    assert(!s2.empty());
    assert(s2.size() == 2); // "hello" is duplicate
    // print_set(s2, "s2 (initializer_list)");
    assert(s2.front() == "hello");
    assert(s2.back() == "world");

    OrderedSet<std::string> s3(s2); // Copy constructor
    assert(!s3.empty());
    assert(s3.size() == 2);
    assert(s3.front() == "hello");
    assert(s3.back() == "world");
    // print_set(s3, "s3 (copy of s2)");

    OrderedSet<std::string> s4(std::move(s2)); // Move constructor
    assert(!s4.empty());
    assert(s4.size() == 2);
    assert(s4.front() == "hello");
    assert(s4.back() == "world");
    // print_set(s4, "s4 (move of s2)");
    assert(s2.empty()); // s2 should be empty after move
    assert(s2.size() == 0);

    OrderedSet<int> s5;
    s5 = s1; // Copy assignment (empty to empty)
    assert(s5.empty());

    s1.insert(1);
    s1.insert(2);
    s5 = s1; // Copy assignment (non-empty)
    assert(s5.size() == 2);
    assert(s5.front() == 1);
    assert(s5.back() == 2);
    // print_set(s5, "s5 (copy assigned from s1)");

    OrderedSet<int> s6;
    s6 = std::move(s1); // Move assignment
    assert(s6.size() == 2);
    assert(s6.front() == 1);
    assert(s6.back() == 2);
    // print_set(s6, "s6 (move assigned from s1)");
    assert(s1.empty());

    std::cout << "Constructors, Empty, Size tests passed." << std::endl;
}

void test_insert() {
    std::cout << "--- Test Insert ---" << std::endl;
    OrderedSet<int> s;
    auto p1 = s.insert(10);
    assert(s.size() == 1);
    assert(*p1.first == 10 && p1.second == true);
    assert(s.contains(10));
    assert(s.front() == 10 && s.back() == 10);

    auto p2 = s.insert(20);
    assert(s.size() == 2);
    assert(*p2.first == 20 && p2.second == true);
    assert(s.contains(20));
    assert(s.front() == 10 && s.back() == 20);

    auto p3 = s.insert(10); // Duplicate
    assert(s.size() == 2);
    assert(*p3.first == 10 && p3.second == false);
    assert(s.front() == 10 && s.back() == 20); // Order unchanged

    // Test insert rvalue
    int val = 30;
    auto p4 = s.insert(std::move(val));
    assert(s.size() == 3);
    assert(*p4.first == 30 && p4.second == true);
    assert(s.contains(30));
    assert(s.back() == 30);
    // print_set(s, "s after inserts");

    std::cout << "Insert tests passed." << std::endl;
}

void test_erase() {
    std::cout << "--- Test Erase ---" << std::endl;
    OrderedSet<std::string> s = {"a", "b", "c", "d", "e"};
    assert(s.size() == 5);

    assert(s.erase("c") == 1); // Erase middle
    assert(s.size() == 4);
    assert(!s.contains("c"));
    // print_set(s, "s after erase 'c'");
    // Expected: a b d e

    auto vec1 = s.as_vector();
    assert(vec1.size() == 4);
    assert(vec1[0] == "a");
    assert(vec1[1] == "b");
    assert(vec1[2] == "d");
    assert(vec1[3] == "e");


    assert(s.erase("a") == 1); // Erase front
    assert(s.size() == 3);
    assert(!s.contains("a"));
    assert(s.front() == "b");
    // print_set(s, "s after erase 'a'");

    assert(s.erase("e") == 1); // Erase back
    assert(s.size() == 2);
    assert(!s.contains("e"));
    assert(s.back() == "d");
    // print_set(s, "s after erase 'e'");

    assert(s.erase("z") == 0); // Erase non-existent
    assert(s.size() == 2);

    s.erase("b");
    s.erase("d");
    assert(s.empty());

    OrderedSet<int> s_int;
    s_int.insert(1);
    s_int.erase(1);
    assert(s_int.empty());

    std::cout << "Erase tests passed." << std::endl;
}

void test_contains_clear() {
    std::cout << "--- Test Contains & Clear ---" << std::endl;
    OrderedSet<int> s;
    s.insert(100);
    s.insert(200);
    assert(s.contains(100));
    assert(s.contains(200));
    assert(!s.contains(300));

    s.clear();
    assert(s.empty());
    assert(s.size() == 0);
    assert(!s.contains(100));
    // print_set(s, "s after clear");

    std::cout << "Contains & Clear tests passed." << std::endl;
}

void test_iterators_and_order() {
    std::cout << "--- Test Iterators & Order ---" << std::endl;
    OrderedSet<int> s;
    s.insert(1);
    s.insert(2);
    s.insert(3);
    s.insert(2); // duplicate

    std::vector<int> forward_expected = {1, 2, 3};
    std::vector<int> forward_actual;
    for (const auto& item : s) {
        forward_actual.push_back(item);
    }
    assert(forward_actual == forward_expected);

    // Test const iterators
    const OrderedSet<int>& cs = s;
    std::vector<int> const_forward_actual;
    for (auto it = cs.cbegin(); it != cs.cend(); ++it) {
        const_forward_actual.push_back(*it);
    }
    assert(const_forward_actual == forward_expected);


    std::vector<int> reverse_expected = {3, 2, 1};
    std::vector<int> reverse_actual;
    for (auto it = s.rbegin(); it != s.rend(); ++it) {
        reverse_actual.push_back(*it);
    }
    assert(reverse_actual == reverse_expected);

    std::vector<int> const_reverse_actual;
    for (auto it = cs.crbegin(); it != cs.crend(); ++it) {
        const_reverse_actual.push_back(*it);
    }
    assert(const_reverse_actual == reverse_expected);


    // Test non-const iterators for modification (if applicable, though OrderedSet values are const-like via map key)
    // For OrderedSet, elements are typically treated as const once inserted regarding their value (as they are keys in a map).
    // However, list iterators themselves can be non-const.
    // Let's just ensure begin()/end() work.
    int sum = 0;
    for (auto it = s.begin(); it != s.end(); ++it) {
        sum += *it; // Reading is fine
    }
    assert(sum == 6);


    OrderedSet<std::string> s_str = {"z", "y", "x"};
    std::string concat_fwd;
    for(const std::string& str : s_str) concat_fwd += str;
    assert(concat_fwd == "zyx");

    std::string concat_rev;
    for(auto it = s_str.rbegin(); it != s_str.rend(); ++it) concat_rev += *it;
    assert(concat_rev == "xyz");


    std::cout << "Iterators & Order tests passed." << std::endl;
}

void test_front_back() {
    std::cout << "--- Test Front & Back ---" << std::endl;
    OrderedSet<int> s;
    bool caught = false;
    try {
        s.front();
    } catch (const std::out_of_range& e) {
        caught = true;
    }
    assert(caught);

    caught = false;
    try {
        s.back();
    } catch (const std::out_of_range& e) {
        caught = true;
    }
    assert(caught);

    s.insert(10);
    assert(s.front() == 10);
    assert(s.back() == 10);

    s.insert(20);
    assert(s.front() == 10);
    assert(s.back() == 20);

    s.insert(5); // Inserted at end
    assert(s.front() == 10);
    assert(s.back() == 5);

    s.erase(10);
    assert(s.front() == 20);
    assert(s.back() == 5);

    // Test const versions
    const OrderedSet<int>& cs = s;
    assert(cs.front() == 20);
    assert(cs.back() == 5);

    // Test mutable versions allow modification if T is not const (Removed as front/back now return const T&)
    // The following lines demonstrated a potential issue if front() returned T&,
    // but since it returns const T&, direct modification is not allowed, which is safer.
    // s.front() = 22; // This would not compile now.

    // Reverting to a safer test that doesn't modify via front()/back() for now.
    // The issue of mutable access needs to be addressed in the class design if it's a hard requirement.
    // For now, testing they return the correct values.
    OrderedSet<int> s_mut;
    s_mut.insert(100);
    s_mut.insert(200);
    const int& front_ref = s_mut.front(); // Changed to const int&
    const int& back_ref = s_mut.back();   // Changed to const int&
    assert(front_ref == 100);
    assert(back_ref == 200);
    // Modifying front_ref or back_ref is not possible as they are const references.
    // This is the desired behavior for safety, similar to std::set.

    std::cout << "Front & Back tests passed." << std::endl;
}

void test_as_vector() {
    std::cout << "--- Test As Vector ---" << std::endl;
    OrderedSet<std::string> s;
    s.insert("one");
    s.insert("two");
    s.insert("three");

    std::vector<std::string> v = s.as_vector();
    assert(v.size() == 3);
    assert(v[0] == "one");
    assert(v[1] == "two");
    assert(v[2] == "three");

    OrderedSet<int> s_empty;
    std::vector<int> v_empty = s_empty.as_vector();
    assert(v_empty.empty());

    std::cout << "As Vector tests passed." << std::endl;
}

void test_merge() {
    std::cout << "--- Test Merge ---" << std::endl;
    OrderedSet<int> s1 = {1, 2, 3};
    OrderedSet<int> s2 = {3, 4, 5};

    OrderedSet<int> s_copy_s1 = s1;
    s_copy_s1.merge(s2); // Merge const&
    // print_set(s_copy_s1, "s_copy_s1 after merge s2");
    std::vector<int> expected1 = {1, 2, 3, 4, 5};
    assert(s_copy_s1.as_vector() == expected1);
    assert(s2.size() == 3); // s2 should be unchanged

    OrderedSet<int> s_move_s1 = {1, 2, 3};
    OrderedSet<int> s_move_s2 = {3, 4, 5};
    s_move_s1.merge(std::move(s_move_s2)); // Merge &&
    // print_set(s_move_s1, "s_move_s1 after merge move s_move_s2");
    assert(s_move_s1.as_vector() == expected1);
    assert(s_move_s2.empty()); // s_move_s2 should be empty

    OrderedSet<std::string> sa = {"apple", "banana"};
    OrderedSet<std::string> sb = {"cherry", "apple", "date"};
    sa.merge(sb);
    std::vector<std::string> expected_str = {"apple", "banana", "cherry", "date"};
    assert(sa.as_vector() == expected_str);
    // print_set(sa, "sa merged with sb");

    std::cout << "Merge tests passed." << std::endl;
}

void test_equality_operators() {
    std::cout << "--- Test Equality Operators ---" << std::endl;
    OrderedSet<int> s1 = {1, 2, 3};
    OrderedSet<int> s2 = {1, 2, 3};
    OrderedSet<int> s3 = {3, 2, 1}; // Same elements, different order
    OrderedSet<int> s4 = {1, 2};    // Different size
    OrderedSet<int> s5 = {1, 2, 4}; // Same size, different elements

    assert(s1 == s2);
    assert(!(s1 != s2));

    assert(s1 != s3); // Order matters
    assert(!(s1 == s3));

    assert(s1 != s4); // Size matters
    assert(!(s1 == s4));

    assert(s1 != s5); // Elements matter
    assert(!(s1 == s5));

    OrderedSet<int> empty1, empty2;
    assert(empty1 == empty2);

    std::cout << "Equality Operators tests passed." << std::endl;
}

// Define Point struct and related functions at file scope or within a namespace
// to avoid issues with local class definitions and friend functions.
struct Point {
    int x, y;
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

// Overload for std::ostream to print Point objects (useful for debugging with print_set)
std::ostream& operator<<(std::ostream& os, const Point& p) {
    os << "(" << p.x << "," << p.y << ")";
    return os;
}

// Hash functor for Point
struct PointHash {
    std::size_t operator()(const Point& p) const {
        auto h1 = std::hash<int>{}(p.x);
        auto h2 = std::hash<int>{}(p.y);
        return h1 ^ (h2 << 1); // Combine hashes
    }
};

void test_custom_hashable_type() {
    std::cout << "--- Test Custom Hashable Type ---" << std::endl;

    OrderedSet<Point, PointHash> points;
    points.insert({1,1});
    points.insert({2,2});
    points.insert({1,1}); // Duplicate

    assert(points.size() == 2);
    assert(points.contains(Point{1,1})); // Use constructor syntax for clarity with assert
    assert(points.contains(Point{2,2}));
    assert(!points.contains(Point{3,3}));

    assert((points.front() == Point{1,1})); // Extra parentheses for assert
    assert((points.back() == Point{2,2}));  // Extra parentheses for assert

    // print_set(points, "Custom type points");
    std::vector<Point> expected_points = {{1,1}, {2,2}};
    assert((points.as_vector() == expected_points)); // Extra parentheses for assert

    std::cout << "Custom Hashable Type tests passed." << std::endl;
}


int main() {
    test_constructors_and_empty_size();
    test_insert();
    test_erase();
    test_contains_clear();
    test_iterators_and_order();
    test_front_back();
    test_as_vector();
    test_merge();
    test_equality_operators();
    test_custom_hashable_type();

    std::cout << "\nAll OrderedSet tests passed!" << std::endl;
    return 0;
}
