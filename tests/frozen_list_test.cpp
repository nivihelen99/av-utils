#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#include "FrozenList.h" // The header for the class we are testing
#include <vector>
#include <string>
#include <list>         // For testing construction from different iterator types
#include <numeric>      // For std::iota
#include <map>          // For testing with more complex types

// Helper to check contents of a FrozenList against a vector
template<typename T>
void require_list_equals_vector(const cpp_collections::FrozenList<T>& fl, const std::vector<T>& vec) {
    REQUIRE(fl.size() == vec.size());
    REQUIRE(fl.empty() == vec.empty());
    for (size_t i = 0; i < fl.size(); ++i) {
        REQUIRE(fl[i] == vec[i]);
        REQUIRE(fl.at(i) == vec.at(i));
    }
    REQUIRE(std::equal(fl.begin(), fl.end(), vec.begin(), vec.end()));
    if (!fl.empty()) {
        REQUIRE(fl.front() == vec.front());
        REQUIRE(fl.back() == vec.back());
    }
}

TEST_CASE("FrozenList Construction", "[FrozenList]") {
    SECTION("Default constructor") {
        cpp_collections::FrozenList<int> fl;
        REQUIRE(fl.empty());
        REQUIRE(fl.size() == 0);
        REQUIRE_THROWS_AS(fl.at(0), std::out_of_range);
        // REQUIRE_THROWS_AS(fl.front(), ...); // Accessing front/back on empty is UB
        // REQUIRE_THROWS_AS(fl.back(), ...);  // Accessing front/back on empty is UB
    }

    SECTION("Constructor with allocator") {
        std::allocator<int> alloc;
        cpp_collections::FrozenList<int> fl(alloc);
        REQUIRE(fl.empty());
        REQUIRE(fl.size() == 0);
        REQUIRE(fl.get_allocator() == alloc);
    }

    SECTION("Constructor with count and value") {
        cpp_collections::FrozenList<int> fl(5, 10);
        std::vector<int> expected_vec(5, 10);
        require_list_equals_vector(fl, expected_vec);

        cpp_collections::FrozenList<std::string> fl_str(3, "test");
        std::vector<std::string> expected_vec_str(3, "test");
        require_list_equals_vector(fl_str, expected_vec_str);
    }
    
    SECTION("Constructor with count and value and allocator") {
        std::allocator<int> alloc;
        cpp_collections::FrozenList<int> fl(5, 10, alloc);
        std::vector<int> expected_vec(5, 10);
        require_list_equals_vector(fl, expected_vec);
        REQUIRE(fl.get_allocator() == alloc);
    }

    SECTION("Constructor from iterators (std::vector)") {
        std::vector<int> source_vec = {1, 2, 3, 4, 5};
        cpp_collections::FrozenList<int> fl(source_vec.begin(), source_vec.end());
        require_list_equals_vector(fl, source_vec);
    }
    
    SECTION("Constructor from iterators (std::list)") {
        std::list<std::string> source_list = {"a", "b", "c"};
        cpp_collections::FrozenList<std::string> fl(source_list.begin(), source_list.end());
        std::vector<std::string> expected_vec = {"a", "b", "c"};
        require_list_equals_vector(fl, expected_vec);
    }

    SECTION("Constructor from iterators (empty range)") {
        std::vector<int> empty_vec;
        cpp_collections::FrozenList<int> fl(empty_vec.begin(), empty_vec.end());
        REQUIRE(fl.empty());
        REQUIRE(fl.size() == 0);
    }
    
    SECTION("Constructor from iterators with allocator") {
        std::vector<int> source_vec = {1, 2, 3};
        std::allocator<int> alloc;
        cpp_collections::FrozenList<int> fl(source_vec.begin(), source_vec.end(), alloc);
        require_list_equals_vector(fl, source_vec);
        REQUIRE(fl.get_allocator() == alloc);
    }

    SECTION("Constructor from initializer_list") {
        cpp_collections::FrozenList<int> fl = {10, 20, 30};
        std::vector<int> expected_vec = {10, 20, 30};
        require_list_equals_vector(fl, expected_vec);

        cpp_collections::FrozenList<int> fl_empty_init = {};
        REQUIRE(fl_empty_init.empty());
    }

    SECTION("Constructor from initializer_list with allocator") {
        std::allocator<int> alloc;
        cpp_collections::FrozenList<int> fl({10, 20, 30}, alloc);
        std::vector<int> expected_vec = {10, 20, 30};
        require_list_equals_vector(fl, expected_vec);
        REQUIRE(fl.get_allocator() == alloc);
    }

    SECTION("Copy constructor") {
        cpp_collections::FrozenList<int> original = {1, 2, 3};
        cpp_collections::FrozenList<int> copy(original);
        require_list_equals_vector(copy, {1, 2, 3});
        REQUIRE(original.data() != copy.data()); // Should be a deep copy

        // Ensure original is not affected by changes to hypothetical mutable copy (not possible here)
        // or by copy's destruction
    }

    SECTION("Copy constructor with allocator") {
        cpp_collections::FrozenList<int> original = {1, 2, 3};
        std::allocator<int> alloc;
        cpp_collections::FrozenList<int> copy(original, alloc);
        require_list_equals_vector(copy, {1, 2, 3});
        REQUIRE(copy.get_allocator() == alloc);
        REQUIRE(original.data() != copy.data());
    }

    SECTION("Move constructor") {
        cpp_collections::FrozenList<int> original = {1, 2, 3, 4, 5};
        // const int* original_data_ptr = original.data(); // Unreliable after move
        size_t original_size = original.size();

        cpp_collections::FrozenList<int> moved_to(std::move(original));
        
        require_list_equals_vector(moved_to, {1, 2, 3, 4, 5});
        // REQUIRE(moved_to.data() == original_data_ptr); // This is true if vector move doesn't reallocate
        REQUIRE(moved_to.size() == original_size);

        // `original` is in a valid but unspecified state.
        // Common behavior for std::vector move is original becomes empty.
        REQUIRE(original.empty()); // Check common behavior
        REQUIRE(original.size() == 0);
    }
    
    SECTION("Move constructor with allocator") {
        std::allocator<int> alloc;
        cpp_collections::FrozenList<int> original = {1, 2, 3};
        size_t original_size = original.size();

        cpp_collections::FrozenList<int> moved_to(std::move(original), alloc);
        
        require_list_equals_vector(moved_to, {1, 2, 3});
        REQUIRE(moved_to.size() == original_size);
        REQUIRE(moved_to.get_allocator() == alloc);
        
        // If allocators are different, a copy might occur. If same, move.
        // Standard guarantees original is valid. If allocators differ, original is unchanged.
        // If allocators are same (or propagate_on_container_move_assignment is true), original is likely empty.
        // Assuming std::allocator, they are typically equal, so original should be empty.
        REQUIRE(original.empty()); 
    }
}

TEST_CASE("FrozenList Element Access", "[FrozenList]") {
    cpp_collections::FrozenList<int> fl = {10, 20, 30, 40};
    const cpp_collections::FrozenList<int>& cfl = fl; // const reference

    SECTION("operator[]") {
        REQUIRE(fl[0] == 10);
        REQUIRE(fl[2] == 30);
        REQUIRE(cfl[0] == 10); // const version
        // UB for out of bounds, not testing
    }

    SECTION("at()") {
        REQUIRE(fl.at(0) == 10);
        REQUIRE(fl.at(3) == 40);
        REQUIRE(cfl.at(1) == 20); // const version
        REQUIRE_THROWS_AS(fl.at(4), std::out_of_range);
        REQUIRE_THROWS_AS(cfl.at(10), std::out_of_range);
    }

    SECTION("front()") {
        REQUIRE(fl.front() == 10);
        REQUIRE(cfl.front() == 10);
    }

    SECTION("back()") {
        REQUIRE(fl.back() == 40);
        REQUIRE(cfl.back() == 40);
    }

    SECTION("data()") {
        const int* ptr = fl.data();
        REQUIRE(ptr[0] == 10);
        REQUIRE(ptr[1] == 20);

        const int* cptr = cfl.data();
        REQUIRE(cptr[0] == 10);
    }
    
    SECTION("Access on empty list") {
        cpp_collections::FrozenList<int> empty_fl;
        const cpp_collections::FrozenList<int>& c_empty_fl = empty_fl;
        REQUIRE_THROWS_AS(empty_fl.at(0), std::out_of_range);
        // front() and back() on empty list is undefined behavior
        // REQUIRE_THROWS(empty_fl.front()); // Cannot reliably test UB
        // REQUIRE_THROWS(empty_fl.back());
        REQUIRE(empty_fl.data() == nullptr); // Or some valid ptr if vector impl does that
        REQUIRE(c_empty_fl.data() == nullptr);
    }
}

TEST_CASE("FrozenList Iterators", "[FrozenList]") {
    cpp_collections::FrozenList<int> fl = {1, 2, 3};
    const cpp_collections::FrozenList<int>& cfl = fl;

    SECTION("begin() and end()") {
        std::vector<int> collected;
        for (auto it = fl.begin(); it != fl.end(); ++it) {
            collected.push_back(*it);
        }
        require_list_equals_vector(fl, collected);
    }

    SECTION("cbegin() and cend()") {
        std::vector<int> collected;
        for (auto it = fl.cbegin(); it != fl.cend(); ++it) {
            collected.push_back(*it);
        }
        require_list_equals_vector(fl, collected);
        
        std::vector<int> c_collected;
        for (auto it = cfl.begin(); it != cfl.end(); ++it) { // cfl.begin() is const_iterator
            c_collected.push_back(*it);
        }
        require_list_equals_vector(cfl, c_collected);
    }
    
    SECTION("Range-based for loop") {
        std::vector<int> collected;
        for(const auto& item : fl) {
            collected.push_back(item);
        }
        require_list_equals_vector(fl, collected);

        std::vector<int> c_collected;
        for(const auto& item : cfl) {
            c_collected.push_back(item);
        }
        require_list_equals_vector(cfl, c_collected);
    }

    SECTION("rbegin() and rend()") {
        std::vector<int> collected_rev;
        for (auto it = fl.rbegin(); it != fl.rend(); ++it) {
            collected_rev.push_back(*it);
        }
        std::vector<int> expected_rev = {3, 2, 1};
        REQUIRE(collected_rev == expected_rev);
    }

    SECTION("crbegin() and crend()") {
        std::vector<int> collected_rev;
        for (auto it = fl.crbegin(); it != fl.crend(); ++it) {
            collected_rev.push_back(*it);
        }
        std::vector<int> expected_rev = {3, 2, 1};
        REQUIRE(collected_rev == expected_rev);

        std::vector<int> c_collected_rev;
        for (auto it = cfl.rbegin(); it != cfl.rend(); ++it) { // cfl.rbegin() is const_reverse_iterator
            c_collected_rev.push_back(*it);
        }
        REQUIRE(c_collected_rev == expected_rev);
    }
    
    SECTION("Iterators on empty list") {
        cpp_collections::FrozenList<int> empty_fl;
        REQUIRE(empty_fl.begin() == empty_fl.end());
        REQUIRE(empty_fl.cbegin() == empty_fl.cend());
        REQUIRE(empty_fl.rbegin() == empty_fl.rend());
        REQUIRE(empty_fl.crbegin() == empty_fl.crend());
    }
}

TEST_CASE("FrozenList Capacity", "[FrozenList]") {
    SECTION("empty() and size()") {
        cpp_collections::FrozenList<int> fl_empty;
        REQUIRE(fl_empty.empty());
        REQUIRE(fl_empty.size() == 0);

        cpp_collections::FrozenList<int> fl_non_empty = {1, 2, 3};
        REQUIRE_FALSE(fl_non_empty.empty());
        REQUIRE(fl_non_empty.size() == 3);
    }

    SECTION("max_size()") {
        cpp_collections::FrozenList<int> fl;
        std::vector<int> vec;
        REQUIRE(fl.max_size() == vec.max_size()); // Should be same as underlying vector
    }
}

TEST_CASE("FrozenList Comparison Operators", "[FrozenList]") {
    cpp_collections::FrozenList<int> fl1 = {1, 2, 3};
    cpp_collections::FrozenList<int> fl2 = {1, 2, 3};
    cpp_collections::FrozenList<int> fl3 = {1, 2, 4};
    cpp_collections::FrozenList<int> fl4 = {1, 2};
    cpp_collections::FrozenList<int> fl_empty;

    SECTION("operator==") {
        REQUIRE(fl1 == fl2);
        REQUIRE_FALSE(fl1 == fl3);
        REQUIRE_FALSE(fl1 == fl4);
        REQUIRE_FALSE(fl1 == fl_empty);
        REQUIRE(cpp_collections::FrozenList<int>() == cpp_collections::FrozenList<int>());
    }

    SECTION("operator!=") {
        REQUIRE_FALSE(fl1 != fl2);
        REQUIRE(fl1 != fl3);
        REQUIRE(fl1 != fl4);
        REQUIRE(fl1 != fl_empty);
    }

    SECTION("operator<") {
        REQUIRE_FALSE(fl1 < fl2); // equal
        REQUIRE(fl1 < fl3);     // {1,2,3} < {1,2,4}
        REQUIRE(fl4 < fl1);     // {1,2} < {1,2,3} (shorter)
        REQUIRE_FALSE(fl3 < fl1);
        REQUIRE(fl_empty < fl1);
        REQUIRE_FALSE(fl1 < fl_empty);
    }

    SECTION("operator<=") {
        REQUIRE(fl1 <= fl2); // equal
        REQUIRE(fl1 <= fl3);
        REQUIRE(fl4 <= fl1);
        REQUIRE_FALSE(fl3 <= fl1);
        REQUIRE(fl_empty <= fl1);
        REQUIRE(fl_empty <= cpp_collections::FrozenList<int>());
    }

    SECTION("operator>") {
        REQUIRE_FALSE(fl1 > fl2); // equal
        REQUIRE_FALSE(fl1 > fl3);
        REQUIRE_FALSE(fl4 > fl1);
        REQUIRE(fl3 > fl1);
        REQUIRE(fl1 > fl4);
        REQUIRE(fl1 > fl_empty);
        REQUIRE_FALSE(fl_empty > fl1);
    }

    SECTION("operator>=") {
        REQUIRE(fl1 >= fl2); // equal
        REQUIRE_FALSE(fl1 >= fl3);
        REQUIRE_FALSE(fl4 >= fl1);
        REQUIRE(fl3 >= fl1);
        REQUIRE(fl1 >= fl4);
        REQUIRE(fl1 >= fl_empty);
        REQUIRE(cpp_collections::FrozenList<int>() >= cpp_collections::FrozenList<int>());
    }
}

TEST_CASE("FrozenList Swap", "[FrozenList]") {
    cpp_collections::FrozenList<int> fl1 = {1, 2, 3};
    cpp_collections::FrozenList<int> fl2 = {10, 20};
    
    std::vector<int> v1 = {1,2,3};
    std::vector<int> v2 = {10,20};

    const int* fl1_data_before = fl1.data();
    const int* fl2_data_before = fl2.data();

    SECTION("Member swap") {
        fl1.swap(fl2);
        require_list_equals_vector(fl1, v2);
        require_list_equals_vector(fl2, v1);
        // Data pointers should also swap if allocators allow (default std::allocator does)
        // Or if vector's swap is efficient
        if (v1.size() > 0 && v2.size() > 0) { // Only check if not empty
             // This check is tricky due to SSO or other optimizations.
             // The important part is content swap.
        }
    }

    SECTION("Non-member swap (std::swap specialization)") {
        using std::swap; // Enable ADL
        swap(fl1, fl2); // Should call our non-member swap
        require_list_equals_vector(fl1, v2);
        require_list_equals_vector(fl2, v1);
    }
}

TEST_CASE("FrozenList std::hash specialization", "[FrozenList]") {
    std::hash<cpp_collections::FrozenList<int>> hasher;
    
    cpp_collections::FrozenList<int> fl1 = {1, 2, 3};
    cpp_collections::FrozenList<int> fl2 = {1, 2, 3};
    cpp_collections::FrozenList<int> fl3 = {3, 2, 1};
    cpp_collections::FrozenList<int> fl_empty;

    REQUIRE(hasher(fl1) == hasher(fl2)); // Equal lists, equal hashes
    // It's possible but not guaranteed that fl1's hash is different from fl3's.
    // A good hash function should make this likely.
    // REQUIRE(hasher(fl1) != hasher(fl3)); // This is not a strict requirement for std::hash compliance
                                         // but good for quality of hash.
    
    // Hash of empty list should be consistent
    REQUIRE(hasher(fl_empty) == hasher(cpp_collections::FrozenList<int>()));

    // Hash involving different types
    std::hash<cpp_collections::FrozenList<std::string>> str_hasher;
    cpp_collections::FrozenList<std::string> fl_str1 = {"a", "b"};
    cpp_collections::FrozenList<std::string> fl_str2 = {"a", "b"};
    cpp_collections::FrozenList<std::string> fl_str3 = {"b", "a"};
    REQUIRE(str_hasher(fl_str1) == str_hasher(fl_str2));
}


TEST_CASE("FrozenList Assignment Operators", "[FrozenList]") {
    SECTION("Copy assignment") {
        cpp_collections::FrozenList<int> fl1 = {1, 2, 3};
        cpp_collections::FrozenList<int> fl2 = {10, 20};
        
        fl1 = fl2; // fl1 now {10, 20}
        require_list_equals_vector(fl1, {10, 20});
        REQUIRE(fl1.size() == 2);
        REQUIRE(fl1[0] == 10);
        
        // Original fl2 should be unchanged
        require_list_equals_vector(fl2, {10, 20});

        // Self-assignment
        fl1 = fl1;
        require_list_equals_vector(fl1, {10, 20});
    }

    SECTION("Move assignment") {
        cpp_collections::FrozenList<int> fl1 = {1, 2, 3};
        cpp_collections::FrozenList<int> fl2 = {10, 20, 30, 40};
        // const int* fl2_data_ptr = fl2.data(); // Unreliable after move

        fl1 = std::move(fl2);
        require_list_equals_vector(fl1, {10, 20, 30, 40});
        // REQUIRE(fl1.data() == fl2_data_ptr); // If vector move was efficient

        // fl2 is in a valid but unspecified state (likely empty for std::vector)
        REQUIRE(fl2.empty());

        // Self-move assignment (should not crash, well-defined)
        // Standard says for vector: "If this == &x, this function does nothing."
        // FrozenList's move assignment uses vector's move assignment.
        cpp_collections::FrozenList<int> fl_self = {5,6,7};
        fl_self = std::move(fl_self);
        require_list_equals_vector(fl_self, {5,6,7}); // Should be unchanged
    }
    
    SECTION("Initializer list assignment") {
        cpp_collections::FrozenList<int> fl1 = {1, 2, 3};
        fl1 = {100, 200, 300, 400, 500};
        require_list_equals_vector(fl1, {100, 200, 300, 400, 500});
        REQUIRE(fl1.size() == 5);

        fl1 = {}; // Assign empty list
        REQUIRE(fl1.empty());
    }
}


TEST_CASE("FrozenList with complex types", "[FrozenList]") {
    SECTION("List of strings") {
        cpp_collections::FrozenList<std::string> fl_str = {"hello", "world", "frozen", "list"};
        std::vector<std::string> vec_str = {"hello", "world", "frozen", "list"};
        require_list_equals_vector(fl_str, vec_str);
    }

    SECTION("List of vectors") {
        using VecInt = std::vector<int>;
        cpp_collections::FrozenList<VecInt> fl_vec = {{1,2}, {3,4,5}, {}};
        std::vector<VecInt> vec_vec = {{1,2}, {3,4,5}, {}};
        require_list_equals_vector(fl_vec, vec_vec);
    }
    
    // Add test for a type that is not copyable but is movable (if FrozenList supports it)
    // For now, FrozenList stores T, so T must be at least copy-constructible or move-constructible
    // for vector to store it. For construction from iterators, it needs to be copyable/movable from
    // the iterator's value type.
}

TEST_CASE("FrozenList Deduction Guides", "[FrozenList][C++17]") {
    // These tests will only compile if deduction guides are working correctly in C++17 or later
    std::vector<int> v_int = {1, 2, 3};
    cpp_collections::FrozenList fl_from_iter(v_int.begin(), v_int.end()); // Deduce FrozenList<int>
    REQUIRE(std::is_same_v<decltype(fl_from_iter), cpp_collections::FrozenList<int, std::allocator<int>>>);
    require_list_equals_vector(fl_from_iter, v_int);

    cpp_collections::FrozenList fl_from_init = {10.0, 20.0, 30.0}; // Deduce FrozenList<double>
    REQUIRE(std::is_same_v<decltype(fl_from_init), cpp_collections::FrozenList<double, std::allocator<double>>>);
    std::vector<double> v_double = {10.0, 20.0, 30.0};
    require_list_equals_vector(fl_from_init, v_double);
    
    cpp_collections::FrozenList fl_from_fill(5u, std::string("fill")); // Deduce FrozenList<std::string>
    REQUIRE(std::is_same_v<decltype(fl_from_fill), cpp_collections::FrozenList<std::string, std::allocator<std::string>>>);
    std::vector<std::string> v_string(5, "fill");
    require_list_equals_vector(fl_from_fill, v_string);


    // With explicit allocator
    std::allocator<long> myAlloc;
    cpp_collections::FrozenList fl_from_iter_alloc(v_int.begin(), v_int.end(), myAlloc); // Deduce FrozenList<int, std::allocator<long>> - wait, this is tricky.
                                                                                       // The allocator is for `long`, but elements are `int`.
                                                                                       // std::vector would rebind. Let's assume T is deduced from iterators.
    // The deduction guide for iterators + allocator is:
    // template<typename InputIt, typename Alloc = std::allocator<typename std::iterator_traits<InputIt>::value_type>>
    // FrozenList(InputIt, InputIt, Alloc = Alloc()) -> FrozenList<typename std::iterator_traits<InputIt>::value_type, Alloc>;
    // So, T is int, Alloc is std::allocator<long>. This means data_ will be std::vector<int, std::allocator<long>>.
    // This is valid if std::allocator_traits<std::allocator<long>>::rebind_alloc<int> is std::allocator<int>.
    // For std::allocator, this is true.

    REQUIRE(std::is_same_v<decltype(fl_from_iter_alloc), cpp_collections::FrozenList<int, std::allocator<long>>>);
    require_list_equals_vector(fl_from_iter_alloc, v_int);
    REQUIRE(fl_from_iter_alloc.get_allocator() == myAlloc);


    cpp_collections::FrozenList fl_from_init_alloc({1L, 2L, 3L}, myAlloc); // Deduce FrozenList<long, std::allocator<long>>
    REQUIRE(std::is_same_v<decltype(fl_from_init_alloc), cpp_collections::FrozenList<long, std::allocator<long>>>);
    std::vector<long> v_long = {1L, 2L, 3L};
    require_list_equals_vector(fl_from_init_alloc, v_long);
    REQUIRE(fl_from_init_alloc.get_allocator() == myAlloc);
}
