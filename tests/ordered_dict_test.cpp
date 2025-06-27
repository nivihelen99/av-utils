#include "gtest/gtest.h"
#include "ordered_dict.h" // Adjust path as necessary
#include <string>
#include <vector>
#include <list> // For std::list specific tests if needed, or type comparisons
#include <algorithm> // For std::is_permutation, std::equal
#include <map> // For comparison against std::map behavior where appropriate

// Basic test fixture
class OrderedDictTest : public ::testing::Test {
protected:
    std_ext::OrderedDict<int, std::string> od_int_str;
    std_ext::OrderedDict<std::string, int> od_str_int;
};

// Test default constructor
TEST_F(OrderedDictTest, DefaultConstructor) {
    EXPECT_TRUE(od_int_str.empty());
    EXPECT_EQ(od_int_str.size(), 0);
    EXPECT_EQ(od_int_str.begin(), od_int_str.end());
}

// Test initializer list constructor
TEST_F(OrderedDictTest, InitializerListConstructor) {
    std_ext::OrderedDict<int, std::string> od = {{1, "one"}, {2, "two"}, {3, "three"}};
    EXPECT_FALSE(od.empty());
    EXPECT_EQ(od.size(), 3);

    // Check order and values
    auto it = od.begin();
    EXPECT_EQ(it->first, 1);
    EXPECT_EQ(it->second, "one");
    ++it;
    EXPECT_EQ(it->first, 2);
    EXPECT_EQ(it->second, "two");
    ++it;
    EXPECT_EQ(it->first, 3);
    EXPECT_EQ(it->second, "three");
    ++it;
    EXPECT_EQ(it, od.end());
}

// Test initializer list constructor with duplicate keys (last one should win and order updated)
TEST_F(OrderedDictTest, InitializerListConstructorDuplicateKeys) {
    std_ext::OrderedDict<int, std::string> od = {{1, "one_first"}, {2, "two"}, {1, "one_last"}};
    EXPECT_EQ(od.size(), 2); // Key 1 is present once

    auto it = od.begin();
    // Order should be 2, then 1 because 1 was updated last
    EXPECT_EQ(it->first, 2);
    EXPECT_EQ(it->second, "two");
    ++it;
    EXPECT_EQ(it->first, 1);
    EXPECT_EQ(it->second, "one_last"); // Value for key 1 should be "one_last"
    ++it;
    EXPECT_EQ(it, od.end());
}


// Test range constructor
TEST_F(OrderedDictTest, RangeConstructor) {
    std::vector<std::pair<const int, std::string>> data = {{10, "ten"}, {20, "twenty"}, {30, "thirty"}};
    std_ext::OrderedDict<int, std::string> od(data.begin(), data.end());
    EXPECT_EQ(od.size(), 3);

    auto it = od.begin();
    EXPECT_EQ(it->first, 10);
    EXPECT_EQ(it->second, "ten");
    ++it;
    EXPECT_EQ(it->first, 20);
    EXPECT_EQ(it->second, "twenty");
    ++it;
    EXPECT_EQ(it->first, 30);
    EXPECT_EQ(it->second, "thirty");
    ++it;
    EXPECT_EQ(it, od.end());
}

// Test copy constructor
TEST_F(OrderedDictTest, CopyConstructor) {
    std_ext::OrderedDict<int, std::string> od1 = {{1, "one"}, {2, "two"}};
    std_ext::OrderedDict<int, std::string> od2 = od1;

    EXPECT_EQ(od1.size(), od2.size());
    EXPECT_TRUE(std::equal(od1.begin(), od1.end(), od2.begin()));

    // Ensure independent copies
    od2.insert({3, "three"});
    EXPECT_NE(od1.size(), od2.size());
    EXPECT_FALSE(od1.contains(3));
    EXPECT_TRUE(od2.contains(3));
}

// Test move constructor
TEST_F(OrderedDictTest, MoveConstructor) {
    std_ext::OrderedDict<int, std::string> od1 = {{1, "one"}, {2, "two"}};
    std_ext::OrderedDict<int, std::string> od_temp = od1; // To compare against
    std_ext::OrderedDict<int, std::string> od2 = std::move(od1);

    EXPECT_EQ(od_temp.size(), od2.size());
    EXPECT_TRUE(std::equal(od_temp.begin(), od_temp.end(), od2.begin()));
    EXPECT_TRUE(od1.empty()); // Moved-from state (usually empty)
}

// Test copy assignment operator
TEST_F(OrderedDictTest, CopyAssignment) {
    std_ext::OrderedDict<int, std::string> od1 = {{1, "one"}, {2, "two"}};
    std_ext::OrderedDict<int, std::string> od2;
    od2 = od1;

    EXPECT_EQ(od1.size(), od2.size());
    EXPECT_TRUE(std::equal(od1.begin(), od1.end(), od2.begin()));

    // Ensure independent copies
    od2.insert({3, "three"});
    EXPECT_NE(od1.size(), od2.size());
}

// Test move assignment operator
TEST_F(OrderedDictTest, MoveAssignment) {
    std_ext::OrderedDict<int, std::string> od1 = {{1, "one"}, {2, "two"}};
    std_ext::OrderedDict<int, std::string> od_temp = od1;
    std_ext::OrderedDict<int, std::string> od2;
    od2 = std::move(od1);

    EXPECT_EQ(od_temp.size(), od2.size());
    EXPECT_TRUE(std::equal(od_temp.begin(), od_temp.end(), od2.begin()));
    EXPECT_TRUE(od1.empty());
}


// Test operator[] for access and insertion
TEST_F(OrderedDictTest, OperatorSquareBrackets) {
    od_int_str[10] = "ten";
    EXPECT_EQ(od_int_str.size(), 1);
    EXPECT_EQ(od_int_str[10], "ten");

    od_int_str[20] = "twenty";
    EXPECT_EQ(od_int_str.size(), 2);
    EXPECT_EQ(od_int_str[20], "twenty");

    od_int_str[10] = "diez"; // Update existing
    EXPECT_EQ(od_int_str.size(), 2);
    EXPECT_EQ(od_int_str[10], "diez");

    // Check order
    auto it = od_int_str.begin();
    EXPECT_EQ(it->first, 10); ++it;
    EXPECT_EQ(it->first, 20); ++it;
    EXPECT_EQ(it, od_int_str.end());
}

// Test at() for access
TEST_F(OrderedDictTest, AtMethod) {
    od_int_str.insert({1, "one"});
    EXPECT_EQ(od_int_str.at(1), "one");

    const auto& const_od = od_int_str;
    EXPECT_EQ(const_od.at(1), "one");

    EXPECT_THROW(od_int_str.at(2), std::out_of_range);
    EXPECT_THROW(const_od.at(2), std::out_of_range);
}

// Test insert
TEST_F(OrderedDictTest, Insert) {
    auto result1 = od_str_int.insert({"apple", 1});
    EXPECT_TRUE(result1.second); // Inserted
    EXPECT_EQ(result1.first->first, "apple");
    EXPECT_EQ(od_str_int.size(), 1);

    auto result2 = od_str_int.insert({"banana", 2});
    EXPECT_TRUE(result2.second);
    EXPECT_EQ(od_str_int.size(), 2);

    auto result3 = od_str_int.insert({"apple", 100}); // Key exists
    EXPECT_FALSE(result3.second); // Not inserted
    EXPECT_EQ(result3.first->second, 1); // Original value
    EXPECT_EQ(od_str_int.size(), 2); // Size unchanged

    // Check order
    auto it = od_str_int.begin();
    EXPECT_EQ(it->first, "apple"); ++it;
    EXPECT_EQ(it->first, "banana"); ++it;
    EXPECT_EQ(it, od_str_int.end());
}

// Test insert_or_assign
TEST_F(OrderedDictTest, InsertOrAssign) {
    auto result1 = od_str_int.insert_or_assign("grape", 3);
    EXPECT_TRUE(result1.second); // Inserted
    EXPECT_EQ(result1.first->second, 3);
    EXPECT_EQ(od_str_int.size(), 1);

    auto result2 = od_str_int.insert_or_assign("grape", 33); // Key exists
    EXPECT_FALSE(result2.second); // Assigned
    EXPECT_EQ(result2.first->second, 33);
    EXPECT_EQ(od_str_int.size(), 1); // Size unchanged

    od_str_int.insert_or_assign("orange", 4);
    // Check order (grape then orange)
    auto it = od_str_int.begin();
    EXPECT_EQ(it->first, "grape"); ++it;
    EXPECT_EQ(it->first, "orange"); ++it;
    EXPECT_EQ(it, od_str_int.end());
}

// Test emplace
TEST_F(OrderedDictTest, Emplace) {
    auto result1 = od_int_str.emplace(1, "one");
    EXPECT_TRUE(result1.second);
    EXPECT_EQ(result1.first->second, "one");

    auto result2 = od_int_str.emplace(1, "another_one");
    EXPECT_FALSE(result2.second); // Not emplaced, key exists
    EXPECT_EQ(result2.first->second, "one"); // Original value
}

// Test try_emplace
TEST_F(OrderedDictTest, TryEmplace) {
    auto result1 = od_int_str.try_emplace(1, "one");
    EXPECT_TRUE(result1.second);
    EXPECT_EQ(result1.first->second, "one");

    std::string val_str = "another_one";
    auto result2 = od_int_str.try_emplace(1, val_str); // try with lvalue
    EXPECT_FALSE(result2.second);
    EXPECT_EQ(result2.first->second, "one");

    auto result3 = od_int_str.try_emplace(2, "two");
    EXPECT_TRUE(result3.second);

    // Check order
    auto it = od_int_str.begin();
    EXPECT_EQ(it->first, 1); ++it;
    EXPECT_EQ(it->first, 2); ++it;
    EXPECT_EQ(it, od_int_str.end());
}


// Test erase by key
TEST_F(OrderedDictTest, EraseByKey) {
    od_int_str = {{1, "a"}, {2, "b"}, {3, "c"}};
    EXPECT_EQ(od_int_str.erase(2), 1); // Erased
    EXPECT_EQ(od_int_str.size(), 2);
    EXPECT_FALSE(od_int_str.contains(2));
    EXPECT_EQ(od_int_str.erase(5), 0); // Not found

    // Check order
    auto it = od_int_str.begin();
    EXPECT_EQ(it->first, 1); ++it;
    EXPECT_EQ(it->first, 3); ++it;
    EXPECT_EQ(it, od_int_str.end());
}

// Test erase by iterator
TEST_F(OrderedDictTest, EraseByIterator) {
    od_int_str = {{1, "a"}, {2, "b"}, {3, "c"}};
    auto it_to_erase = od_int_str.find(2);
    auto next_it = od_int_str.erase(it_to_erase);

    EXPECT_EQ(od_int_str.size(), 2);
    EXPECT_FALSE(od_int_str.contains(2));
    ASSERT_NE(next_it, od_int_str.end());
    EXPECT_EQ(next_it->first, 3); // Iterator to next element

    // Erase first element
    next_it = od_int_str.erase(od_int_str.begin());
    EXPECT_EQ(od_int_str.size(), 1);
    EXPECT_FALSE(od_int_str.contains(1));
    ASSERT_NE(next_it, od_int_str.end());
    EXPECT_EQ(next_it->first, 3); // Was 3, now it's the new begin

    // Erase last element
    next_it = od_int_str.erase(od_int_str.begin()); // Now only {3, "c"} is left
    EXPECT_EQ(od_int_str.size(), 0);
    EXPECT_TRUE(od_int_str.empty());
    EXPECT_EQ(next_it, od_int_str.end());
}

// Test clear
TEST_F(OrderedDictTest, Clear) {
    od_int_str = {{1, "a"}, {2, "b"}};
    EXPECT_FALSE(od_int_str.empty());
    od_int_str.clear();
    EXPECT_TRUE(od_int_str.empty());
    EXPECT_EQ(od_int_str.size(), 0);
}

// Test swap
TEST_F(OrderedDictTest, Swap) {
    std_ext::OrderedDict<int, std::string> od1 = {{1, "one"}, {2, "two"}};
    std_ext::OrderedDict<int, std::string> od2 = {{10, "ten"}, {20, "twenty"}, {30, "thirty"}};

    std_ext::OrderedDict<int, std::string> od1_orig = od1;
    std_ext::OrderedDict<int, std::string> od2_orig = od2;

    od1.swap(od2);

    EXPECT_EQ(od1.size(), od2_orig.size());
    EXPECT_TRUE(std::equal(od1.begin(), od1.end(), od2_orig.begin()));
    EXPECT_EQ(od2.size(), od1_orig.size());
    EXPECT_TRUE(std::equal(od2.begin(), od2.end(), od1_orig.begin()));
}

// Test find
TEST_F(OrderedDictTest, Find) {
    od_int_str = {{1, "a"}, {2, "b"}};
    auto it_found = od_int_str.find(1);
    ASSERT_NE(it_found, od_int_str.end());
    EXPECT_EQ(it_found->second, "a");

    auto it_not_found = od_int_str.find(3);
    EXPECT_EQ(it_not_found, od_int_str.end());

    const auto& const_od = od_int_str;
    auto const_it_found = const_od.find(2);
    ASSERT_NE(const_it_found, const_od.end());
    EXPECT_EQ(const_it_found->second, "b");
}

// Test count and contains
TEST_F(OrderedDictTest, CountAndContains) {
    od_int_str = {{1, "a"}, {2, "b"}};
    EXPECT_EQ(od_int_str.count(1), 1);
    EXPECT_TRUE(od_int_str.contains(1));
    EXPECT_EQ(od_int_str.count(3), 0);
    EXPECT_FALSE(od_int_str.contains(3));
}

// Test empty, size, max_size
TEST_F(OrderedDictTest, CapacityMethods) {
    EXPECT_TRUE(od_int_str.empty());
    EXPECT_EQ(od_int_str.size(), 0);

    od_int_str.insert({1, "a"});
    EXPECT_FALSE(od_int_str.empty());
    EXPECT_EQ(od_int_str.size(), 1);
    EXPECT_GT(od_int_str.max_size(), 0); // Should be some large number
}

// Test iterators (begin, end, cbegin, cend, reverse)
TEST_F(OrderedDictTest, Iterators) {
    od_int_str = {{1, "a"}, {2, "b"}, {3, "c"}};
    std::vector<int> forward_keys, reverse_keys;
    std::vector<int> expected_forward = {1, 2, 3};
    std::vector<int> expected_reverse = {3, 2, 1};

    for(const auto& pair : od_int_str) {
        forward_keys.push_back(pair.first);
    }
    EXPECT_EQ(forward_keys, expected_forward);

    for(auto it = od_int_str.rbegin(); it != od_int_str.rend(); ++it) {
        reverse_keys.push_back(it->first);
    }
    EXPECT_EQ(reverse_keys, expected_reverse);

    // Const iterators
    const auto& const_od = od_int_str;
    forward_keys.clear();
    for(const auto& pair : const_od) { // uses cbegin/cend
        forward_keys.push_back(pair.first);
    }
    EXPECT_EQ(forward_keys, expected_forward);
}


// Test popitem
TEST_F(OrderedDictTest, PopItem) {
    od_int_str = {{1, "one"}, {2, "two"}, {3, "three"}};

    // Pop last
    auto item1 = od_int_str.popitem(); // last = true by default
    EXPECT_EQ(item1.first, 3);
    EXPECT_EQ(item1.second, "three");
    EXPECT_EQ(od_int_str.size(), 2);
    EXPECT_FALSE(od_int_str.contains(3));
    // Check order: 1, 2
    auto it_check1 = od_int_str.begin();
    EXPECT_EQ(it_check1->first, 1); ++it_check1;
    EXPECT_EQ(it_check1->first, 2); ++it_check1;
    EXPECT_EQ(it_check1, od_int_str.end());

    // Pop first
    auto item2 = od_int_str.popitem(false); // last = false
    EXPECT_EQ(item2.first, 1);
    EXPECT_EQ(item2.second, "one");
    EXPECT_EQ(od_int_str.size(), 1);
    EXPECT_FALSE(od_int_str.contains(1));
    // Check order: 2
    auto it_check2 = od_int_str.begin();
    EXPECT_EQ(it_check2->first, 2); ++it_check2;
    EXPECT_EQ(it_check2, od_int_str.end());

    // Pop last remaining item
    auto item3 = od_int_str.popitem();
    EXPECT_EQ(item3.first, 2);
    EXPECT_EQ(item3.second, "two");
    EXPECT_TRUE(od_int_str.empty());

    // Pop from empty
    EXPECT_THROW(od_int_str.popitem(), std::out_of_range);
}

// Test with custom types and hashers (if applicable, though OrderedDict is generic)
struct MyKey {
    int id;
    std::string name;
    bool operator==(const MyKey& other) const {
        return id == other.id && name == other.name;
    }
};

struct MyKeyHash {
    std::size_t operator()(const MyKey& k) const {
        return std::hash<int>()(k.id) ^ (std::hash<std::string>()(k.name) << 1);
    }
};

TEST_F(OrderedDictTest, CustomKeyType) {
    std_ext::OrderedDict<MyKey, int, MyKeyHash> od_custom;
    od_custom.insert({{1, "apple"}, 10});
    od_custom.insert_or_assign({2, "banana"}, 20);
    od_custom[{1, "apple"}] = 100; // Update

    EXPECT_EQ(od_custom.size(), 2);
    EXPECT_TRUE(od_custom.contains({1, "apple"}));
    EXPECT_EQ(od_custom.at({1, "apple"}), 100);

    // Check order
    auto it = od_custom.begin();
    EXPECT_EQ(it->first.id, 1);
    EXPECT_EQ(it->first.name, "apple");
    ++it;
    EXPECT_EQ(it->first.id, 2);
    EXPECT_EQ(it->first.name, "banana");
    ++it;
    EXPECT_EQ(it, od_custom.end());
}

// Test equality operator
TEST_F(OrderedDictTest, EqualityOperator) {
    std_ext::OrderedDict<int, std::string> od1 = {{1, "one"}, {2, "two"}};
    std_ext::OrderedDict<int, std::string> od2 = {{1, "one"}, {2, "two"}};
    std_ext::OrderedDict<int, std::string> od3 = {{2, "two"}, {1, "one"}}; // Different order
    std_ext::OrderedDict<int, std::string> od4 = {{1, "one"}, {2, "zwei"}}; // Different value
    std_ext::OrderedDict<int, std::string> od5 = {{1, "one"}}; // Different size

    EXPECT_TRUE(od1 == od2);
    EXPECT_FALSE(od1 == od3);
    EXPECT_FALSE(od1 == od4);
    EXPECT_FALSE(od1 == od5);

    EXPECT_FALSE(od1 != od2);
    EXPECT_TRUE(od1 != od3);
}

// Test allocator support (basic check: construction with allocator)
TEST_F(OrderedDictTest, AllocatorSupport) {
    using MyAllocator = std::allocator<std::pair<const int, std::string>>;
    MyAllocator my_alloc;
    std_ext::OrderedDict<int, std::string, std::hash<int>, std::equal_to<int>, MyAllocator> od_alloc(my_alloc);
    od_alloc.insert({1, "hello"});
    EXPECT_EQ(od_alloc.size(), 1);
    EXPECT_EQ(od_alloc.get_allocator(), my_alloc);

    std_ext::OrderedDict<int, std::string, std::hash<int>, std::equal_to<int>, MyAllocator> od_alloc_range(
        {{1,"a"},{2,"b"}}, 0, std::hash<int>(), std::equal_to<int>(), my_alloc);
    EXPECT_EQ(od_alloc_range.size(), 2);
    EXPECT_EQ(od_alloc_range.get_allocator(), my_alloc);
}

// Test edge cases: Erasing non-existent elements
TEST_F(OrderedDictTest, EraseNonExistent) {
    od_int_str[1] = "one";
    EXPECT_EQ(od_int_str.erase(2), 0); // Erase non-existent key
    EXPECT_EQ(od_int_str.size(), 1);
}

// Test edge cases: Operations on empty dictionary
TEST_F(OrderedDictTest, OperationsOnEmpty) {
    EXPECT_EQ(od_int_str.find(1), od_int_str.end());
    EXPECT_EQ(od_int_str.count(1), 0);
    EXPECT_FALSE(od_int_str.contains(1));
    EXPECT_THROW(od_int_str.at(1), std::out_of_range);
    EXPECT_THROW(od_int_str.popitem(), std::out_of_range);
    auto it = od_int_str.erase(od_int_str.begin()); // Erase on empty (begin() == end())
    EXPECT_EQ(it, od_int_str.end());
}

// Test insertion order preservation with multiple operations
TEST_F(OrderedDictTest, ComplexOrderPreservation) {
    od_int_str[1] = "A"; // {1:A}
    od_int_str[2] = "B"; // {1:A, 2:B}
    od_int_str.insert({0, "Z"}); // {1:A, 2:B, 0:Z}
    od_int_str[1] = "AA"; // {1:AA, 2:B, 0:Z} (order of 1 unchanged by update)
    od_int_str.erase(2);  // {1:AA, 0:Z}
    od_int_str.insert_or_assign(3, "C"); // {1:AA, 0:Z, 3:C}
    od_int_str.try_emplace(-1, "Neg"); // {1:AA, 0:Z, 3:C, -1:Neg}
    od_int_str.insert_or_assign(0, "ZZ"); // {1:AA, 0:ZZ, 3:C, -1:Neg} (order of 0 unchanged)

    std::vector<int> expected_keys = {1, 0, 3, -1};
    std::vector<std::string> expected_values = {"AA", "ZZ", "C", "Neg"};

    std::vector<int> actual_keys;
    std::vector<std::string> actual_values;
    for(const auto& p : od_int_str) {
        actual_keys.push_back(p.first);
        actual_values.push_back(p.second);
    }
    EXPECT_EQ(actual_keys, expected_keys);
    EXPECT_EQ(actual_values, expected_values);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
