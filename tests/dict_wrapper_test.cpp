#include "dict_wrapper.h" // Adjust path as necessary
#include <gtest/gtest.h>
#include <stdexcept> // For std::out_of_range
#include <string>
#include <vector>

// Test fixture for DictWrapper
class DictWrapperTest : public ::testing::Test {
protected:
  DictWrapper<std::string, int> dict;
  DictWrapper<int, std::string> dict_int_str;
};

// Test basic construction and emptiness
TEST_F(DictWrapperTest, ConstructorAndEmpty) {
  EXPECT_TRUE(dict.empty());
  EXPECT_EQ(0, dict.size());

  DictWrapper<std::string, int> dict_init_list = {{"one", 1}, {"two", 2}};
  EXPECT_FALSE(dict_init_list.empty());
  EXPECT_EQ(2, dict_init_list.size());
}

// Test insertion and operator[]
TEST_F(DictWrapperTest, InsertAndBrackets) {
  auto result = dict.insert({"one", 1});
  EXPECT_TRUE(result.second); // Successful insertion
  EXPECT_EQ("one", result.first->first);
  EXPECT_EQ(1, result.first->second);
  EXPECT_EQ(1, dict.size());

  dict["two"] = 2;
  EXPECT_EQ(2, dict.size());
  EXPECT_EQ(1, dict["one"]);
  EXPECT_EQ(2, dict["two"]);

  // Test insert existing
  auto result_exist = dict.insert({"one", 10});
  EXPECT_FALSE(result_exist.second); // Failed insertion (already exists)
  EXPECT_EQ(1,
            result_exist.first->second); // Iterator points to existing element
  EXPECT_EQ(2, dict.size());             // Size should not change
  EXPECT_EQ(1, dict["one"]);             // Value should not change
}

// Test `at` and `contains`
TEST_F(DictWrapperTest, AtAndContains) {
  dict["one"] = 1;
  EXPECT_EQ(1, dict.at("one"));
  EXPECT_TRUE(dict.contains("one"));
  EXPECT_FALSE(dict.contains("two"));

  EXPECT_THROW(dict.at("two"), std::out_of_range);

  const auto &const_dict = dict;
  EXPECT_EQ(1, const_dict.at("one"));
  EXPECT_TRUE(const_dict.contains("one"));
  EXPECT_THROW(const_dict.at("two"), std::out_of_range);
}

// Test erase
TEST_F(DictWrapperTest, Erase) {
  dict["one"] = 1;
  dict["two"] = 2;
  dict["three"] = 3;
  EXPECT_EQ(3, dict.size());

  // Erase by key
  EXPECT_EQ(1, dict.erase("two")); // Erase existing
  EXPECT_EQ(2, dict.size());
  EXPECT_FALSE(dict.contains("two"));
  EXPECT_EQ(0, dict.erase("two")); // Erase non-existing

  // Erase by iterator
  auto it = dict.find("one");
  ASSERT_NE(dict.end(), it);
  it = dict.erase(it); // Erase 'one'
  EXPECT_EQ(1, dict.size());
  EXPECT_FALSE(dict.contains("one"));
  if (it != dict.end()) { // 'it' might be end() or point to 'three' depending
                          // on map impl.
    // In this case, with "three" remaining, it should point to "three" or end()
    // For std::unordered_map, erase invalidates iterators, so we can't rely on
    // it directly. A better test for erase(iterator) would check size and
    // remaining elements.
  }

  // Erase by range (if applicable, though simple erase is more common)
  dict["four"] = 4;
  dict["five"] = 5;
  // To test erase(first, last), we need sorted iterators or specific known
  // order. For unordered_map, this is tricky. Let's clear and re-add for a
  // simple range.
  dict.clear();
  dict["a"] = 1;
  dict["b"] = 2;
  dict["c"] = 3;
  dict["d"] = 4;
  // Erase all elements
  dict.erase(dict.begin(), dict.end());
  EXPECT_TRUE(dict.empty());
}

// Test clear and iterators
TEST_F(DictWrapperTest, ClearAndIterators) {
  dict["one"] = 1;
  dict["two"] = 2;
  ASSERT_FALSE(dict.empty());

  dict.clear();
  EXPECT_TRUE(dict.empty());
  EXPECT_EQ(0, dict.size());
  EXPECT_EQ(dict.begin(), dict.end());
  EXPECT_EQ(dict.cbegin(), dict.cend());

  // Check iterators on an empty map
  DictWrapper<int, int> empty_dict;
  EXPECT_EQ(empty_dict.begin(), empty_dict.end());
}

// Test count
TEST_F(DictWrapperTest, Count) {
  dict["one"] = 1;
  EXPECT_EQ(1, dict.count("one"));
  EXPECT_EQ(0, dict.count("two"));
  dict.insert({"one", 10});        // Should not insert
  EXPECT_EQ(1, dict.count("one")); // Still 1 for unique maps
}

// Test emplace
TEST_F(DictWrapperTest, Emplace) {
  auto result = dict.emplace("one", 1);
  EXPECT_TRUE(result.second);
  EXPECT_EQ("one", result.first->first);
  EXPECT_EQ(1, result.first->second);
  EXPECT_EQ(1, dict.size());

  result = dict.emplace("one", 10); // Attempt to emplace existing
  EXPECT_FALSE(result.second);
  EXPECT_EQ(1, result.first->second); // Value remains original
  EXPECT_EQ(1, dict.size());
}

// Test swap
TEST_F(DictWrapperTest, Swap) {
  DictWrapper<std::string, int> dict1 = {{"a", 1}, {"b", 2}};
  DictWrapper<std::string, int> dict2 = {{"x", 10}, {"y", 20}, {"z", 30}};

  dict1.swap(dict2);

  EXPECT_EQ(3, dict1.size());
  EXPECT_TRUE(dict1.contains("x"));
  EXPECT_FALSE(dict1.contains("a"));

  EXPECT_EQ(2, dict2.size());
  EXPECT_TRUE(dict2.contains("a"));
  EXPECT_FALSE(dict2.contains("x"));

  swap(dict1, dict2); // Test non-member swap

  EXPECT_EQ(2, dict1.size());
  EXPECT_TRUE(dict1.contains("a"));
  EXPECT_FALSE(dict1.contains("x"));

  EXPECT_EQ(3, dict2.size());
  EXPECT_TRUE(dict2.contains("x"));
  EXPECT_FALSE(dict2.contains("a"));
}

// Test equality operators
TEST_F(DictWrapperTest, EqualityOperators) {
  DictWrapper<std::string, int> dict1 = {{"a", 1}, {"b", 2}};
  DictWrapper<std::string, int> dict2 = {
      {"b", 2}, {"a", 1}}; // Same elements, different order
  DictWrapper<std::string, int> dict3 = {{"a", 1}, {"b", 3}}; // Different value
  DictWrapper<std::string, int> dict4 = {{"a", 1}};           // Different size
  DictWrapper<std::string, int> dict1_copy = dict1;

  EXPECT_TRUE(dict1 == dict1_copy);
  EXPECT_TRUE(dict1 ==
              dict2); // Order doesn't matter for unordered_map comparison
  EXPECT_FALSE(dict1 == dict3);
  EXPECT_FALSE(dict1 == dict4);

  EXPECT_FALSE(dict1 != dict1_copy);
  EXPECT_FALSE(dict1 != dict2);
  EXPECT_TRUE(dict1 != dict3);
  EXPECT_TRUE(dict1 != dict4);
}

// --- Tests for derived class overriding behavior ---

// Custom dictionary for testing override
class MyCustomDict : public DictWrapper<std::string, int> {
public:
  using Base = DictWrapper<std::string, int>;
  int insert_override_called = 0;
  int erase_override_called = 0;
  int at_override_called = 0;
  int bracket_override_called = 0;

  using Base::Base; // Inherit constructors

  std::pair<iterator, bool> insert(const value_type &value) override {
    insert_override_called++;
    return Base::insert(value);
  }

  size_type erase(const key_type &key) override {
    erase_override_called++;
    return Base::erase(key);
  }

  mapped_type &at(const key_type &key) override {
    at_override_called++;
    return Base::at(key);
  }

  // Note: const version of 'at' also needs to be overridden if used.
  const mapped_type &at(const key_type &key) const override {
    // To make this testable, we'd need a mutable counter or a way to track
    // calls in const methods. For simplicity, we'll focus on non-const `at` or
    // use a trick. const_cast<MyCustomDict*>(this)->at_override_called++; //
    // Not ideal, but for testing... Let's just rely on the non-const version
    // for this simple test.
    return Base::at(key);
  }

  mapped_type &operator[](const key_type &key) override {
    bracket_override_called++;
    return Base::operator[](key);
  }
};

TEST_F(DictWrapperTest, DerivedClassOverrideInsert) {
  MyCustomDict custom_dict;
  custom_dict.insert({"hello", 100});
  EXPECT_EQ(1, custom_dict.insert_override_called);
  EXPECT_EQ(
      100,
      custom_dict["hello"]); // operator[] might also call overridden version
  EXPECT_EQ(
      1, custom_dict.bracket_override_called); // Reset for next specific tests
  custom_dict.bracket_override_called = 0;

  custom_dict.insert({"hello", 200}); // Attempt duplicate
  EXPECT_EQ(2, custom_dict.insert_override_called);
  EXPECT_EQ(100, custom_dict["hello"]); // Value should remain 100
  EXPECT_EQ(1, custom_dict.bracket_override_called);
}

TEST_F(DictWrapperTest, DerivedClassOverrideErase) {
  MyCustomDict custom_dict;
  custom_dict["world"] = 200;
  custom_dict.bracket_override_called = 0; // Reset after setup

  custom_dict.erase("world");
  EXPECT_EQ(1, custom_dict.erase_override_called);
  EXPECT_FALSE(custom_dict.contains("world"));

  custom_dict.erase("nonexistent");
  EXPECT_EQ(2, custom_dict.erase_override_called);
}

TEST_F(DictWrapperTest, DerivedClassOverrideAt) {
  MyCustomDict custom_dict;
  custom_dict["key1"] = 300;
  custom_dict.bracket_override_called = 0; // Reset

  EXPECT_EQ(300, custom_dict.at("key1"));
  EXPECT_EQ(1, custom_dict.at_override_called);

  EXPECT_THROW(custom_dict.at("nonexistent"), std::out_of_range);
  EXPECT_EQ(2,
            custom_dict.at_override_called); // `at` is called even if it throws
}

TEST_F(DictWrapperTest, DerivedClassOverrideBrackets) {
  MyCustomDict custom_dict;
  custom_dict["newkey"] = 400; // Insertion
  EXPECT_EQ(1, custom_dict.bracket_override_called);
  EXPECT_EQ(400, custom_dict.at("newkey")); // Check value using public accessor
  EXPECT_EQ(1, custom_dict.at_override_called); // at() was called
  custom_dict.at_override_called = 0;           // Reset for next check

  custom_dict["newkey"] = 401; // Access and assignment
  EXPECT_EQ(2, custom_dict.bracket_override_called);
  EXPECT_EQ(401, custom_dict.at("newkey")); // Check value
  EXPECT_EQ(1, custom_dict.at_override_called);
  custom_dict.at_override_called = 0; // Reset

  // Access for reading
  [[maybe_unused]] int val = custom_dict["newkey"];
  EXPECT_EQ(3, custom_dict.bracket_override_called);
}

// Test iterators with a derived class (should behave like base)
TEST_F(DictWrapperTest, DerivedClassIterators) {
  MyCustomDict custom_dict = {{"a", 1}, {"b", 2}};
  custom_dict.insert_override_called = 0; // Reset counter

  std::vector<std::pair<std::string, int>> contents;
  for (const auto &p : custom_dict) {
    contents.push_back(p);
  }
  EXPECT_EQ(2, contents.size());
  // Check if elements exist, order doesn't matter for unordered_map
  bool found_a = false, found_b = false;
  for (const auto &p : contents) {
    if (p.first == "a" && p.second == 1)
      found_a = true;
    if (p.first == "b" && p.second == 2)
      found_b = true;
  }
  EXPECT_TRUE(found_a);
  EXPECT_TRUE(found_b);

  // Ensure base iterators are used (no override calls for begin/end)
  EXPECT_EQ(
      0,
      custom_dict.insert_override_called); // No insert calls during iteration
}

// Test const correctness with derived class
TEST_F(DictWrapperTest, DerivedClassConstMethods) {
  MyCustomDict custom_dict_mut;
  custom_dict_mut["const_test"] = 500;
  custom_dict_mut.bracket_override_called = 0; // Reset for this test

  const MyCustomDict &const_ref_dict = custom_dict_mut;

  EXPECT_TRUE(const_ref_dict.contains("const_test"));
  EXPECT_EQ(500, const_ref_dict.at("const_test"));
  // If const `at` was overridden to increment a mutable counter, we could test
  // it here. Since it's not, we just check it works.

  EXPECT_EQ(1, const_ref_dict.size());
  EXPECT_FALSE(const_ref_dict.empty());

  // Test iteration on const derived object
  int count = 0;
  for (const auto &pair : const_ref_dict) {
    if (pair.first == "const_test" && pair.second == 500) {
      count++;
    }
  }
  EXPECT_EQ(1, count);
  EXPECT_EQ(
      0,
      custom_dict_mut.bracket_override_called); // No non-const operator[] calls
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
