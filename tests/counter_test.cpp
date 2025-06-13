#include "gtest/gtest.h"
#include "counter.h"
#include <string>
#include <vector>
#include <map>
#include <algorithm> // For std::sort, std::find_if
#include <set>       // For testing with std::set as keys

// Minimal Person struct for testing custom types
struct TestPerson {
    std::string name;
    int age;

    TestPerson() : age(0) {}
    TestPerson(std::string n, int a) : name(std::move(n)), age(a) {}

    bool operator==(const TestPerson& other) const {
        return name == other.name && age == other.age;
    }

    bool operator<(const TestPerson& other) const { // Needed for most_common tie-breaking and std::map keys
        if (name != other.name) {
            return name < other.name;
        }
        return age < other.age;
    }
};

// Custom output operator for TestPerson to help with GTest failure messages
std::ostream& operator<<(std::ostream& os, const TestPerson& p) {
    return os << "TestPerson(\"" << p.name << "\", " << p.age << ")";
}

struct TestPersonHash {
    std::size_t operator()(const TestPerson& p) const {
        return std::hash<std::string>{}(p.name) ^ (std::hash<int>{}(p.age) << 1);
    }
};


// Test Fixture for common setup
class CounterTest : public ::testing::Test {
protected:
    Counter<std::string> c_str;
    Counter<int> c_int;
};

// 1. Constructors
TEST_F(CounterTest, BasicInstantiation) {
    Counter<int> c1;
    EXPECT_TRUE(c1.empty());
    EXPECT_EQ(c1.size(), 0);

    Counter<std::string> c2;
    EXPECT_TRUE(c2.empty());
    EXPECT_EQ(c2.size(), 0);
}

TEST_F(CounterTest, InitializerListT) {
    Counter<std::string> c{"a", "b", "a", "c", "a"};
    EXPECT_EQ(c.count("a"), 3);
    EXPECT_EQ(c.count("b"), 1);
    EXPECT_EQ(c.count("c"), 1);
    EXPECT_EQ(c.size(), 3);
}

TEST_F(CounterTest, InitializerListPairTInt) {
    Counter<std::string> c{{"a", 2}, {"b", 1}, {"c", 0}, {"d", -1}}; // Zero and negative should be ignored by this constructor
    EXPECT_EQ(c.count("a"), 2);
    EXPECT_EQ(c.count("b"), 1);
    EXPECT_FALSE(c.contains("c")); // 0 count pair should not add element
    EXPECT_FALSE(c.contains("d")); // -1 count pair should not add element
    EXPECT_EQ(c.size(), 2);
}

TEST_F(CounterTest, IteratorsConstructor) {
    std::vector<int> v = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4};
    Counter<int> c(v.begin(), v.end());
    EXPECT_EQ(c.count(1), 1);
    EXPECT_EQ(c.count(2), 2);
    EXPECT_EQ(c.count(3), 3);
    EXPECT_EQ(c.count(4), 4);
    EXPECT_EQ(c.size(), 4);

    std::set<std::string> s = {"apple", "banana", "apple"}; // Set will have unique elements
    Counter<std::string> c_set(s.begin(), s.end());
    EXPECT_EQ(c_set.count("apple"), 1);
    EXPECT_EQ(c_set.count("banana"), 1);
    EXPECT_EQ(c_set.size(), 2);
}

TEST_F(CounterTest, CopyConstructor) {
    c_str.add("a", 5);
    c_str.add("b", 3);
    Counter<std::string> c_copy(c_str);

    EXPECT_EQ(c_copy.count("a"), 5);
    EXPECT_EQ(c_copy.count("b"), 3);
    EXPECT_EQ(c_copy.size(), 2);
    EXPECT_FALSE(c_copy.empty());
    // Ensure it's a deep copy
    c_copy.add("a", 1);
    EXPECT_EQ(c_str.count("a"), 5); // Original unchanged
    EXPECT_EQ(c_copy.count("a"), 6);
}

TEST_F(CounterTest, MoveConstructor) {
    Counter<std::string> c1;
    c1.add("x", 10);
    c1.add("y", 20);

    Counter<std::string> c2(std::move(c1));

    EXPECT_EQ(c2.count("x"), 10);
    EXPECT_EQ(c2.count("y"), 20);
    EXPECT_EQ(c2.size(), 2);
    // Per standard, moved-from object is in a valid but unspecified state.
    // We can check common behaviors like empty or size 0.
    // EXPECT_TRUE(c1.empty()); // This is a common state but not guaranteed.
}

// 2. Core Operations
TEST_F(CounterTest, AddConstLValue) {
    c_int.add(1, 3);
    c_int.add(2);    // count = 1 by default
    c_int.add(1, 2);
    EXPECT_EQ(c_int.count(1), 5);
    EXPECT_EQ(c_int.count(2), 1);

    // Add with negative count (should call subtract)
    c_int.add(1, -2);
    EXPECT_EQ(c_int.count(1), 3);
    c_int.add(2, -1); // Should remove 2
    EXPECT_FALSE(c_int.contains(2));
}

TEST_F(CounterTest, AddRValue) {
    std::string s1 = "hello";
    c_str.add(std::move(s1));
    c_str.add("world", 2);
    std::string s2 = "hello";
    c_str.add(std::move(s2), 2);

    EXPECT_EQ(c_str.count("hello"), 3);
    EXPECT_EQ(c_str.count("world"), 2);
    EXPECT_TRUE(s1.empty() || s1 != "hello"); // s1 is in a valid but unspecified state after move
    // s2's state after std::move into map operator[] where key "hello" already existed:
    // The string s2 itself is not guaranteed to be modified (e.g. cleared) because operator[]
    // might not have constructed a new element with it if "hello" was already a key.
    // The important part is that the counter itself is correct.
    // No assertion on s2's state for now, or a more complex one if specific map behavior is targeted.
}

TEST_F(CounterTest, Subtract) {
    c_int.add(1, 10);
    c_int.add(2, 5);

    c_int.subtract(1, 3);
    EXPECT_EQ(c_int.count(1), 7);

    c_int.subtract(1, 7); // Count becomes 0, item removed
    EXPECT_EQ(c_int.count(1), 0);
    EXPECT_FALSE(c_int.contains(1));
    EXPECT_EQ(c_int.size(), 1);

    c_int.subtract(2, 10); // Count becomes negative, item removed
    EXPECT_EQ(c_int.count(2), 0);
    EXPECT_FALSE(c_int.contains(2));
    EXPECT_TRUE(c_int.empty());

    c_int.subtract(3, 1); // Subtract non-existent item
    EXPECT_FALSE(c_int.contains(3));

    c_int.add(4,5);
    c_int.subtract(4,0); // Subtract zero
    EXPECT_EQ(c_int.count(4),5);
    c_int.subtract(4,-2); // Subtract negative (should do nothing)
    EXPECT_EQ(c_int.count(4),5);
}

TEST_F(CounterTest, CountAndConstOperatorBracket) {
    c_str.add("a", 3);
    EXPECT_EQ(c_str.count("a"), 3);
    EXPECT_EQ(c_str["a"], 3);
    EXPECT_EQ(c_str.count("b"), 0); // Non-existent
    EXPECT_EQ(c_str["b"], 0);     // Non-existent
}

TEST_F(CounterTest, NonConstOperatorBracket) {
    c_str["new_key"] = 5; // Assign to new key
    EXPECT_EQ(c_str.count("new_key"), 5);

    c_str.add("existing", 2);
    c_str["existing"]++;     // Increment existing
    EXPECT_EQ(c_str.count("existing"), 3);

    c_str["another_new"]; // Access new key, should default to 0
    EXPECT_EQ(c_str.count("another_new"), 0);
    EXPECT_TRUE(c_str.contains("another_new"));
}

TEST_F(CounterTest, Contains) {
    c_int.add(100);
    EXPECT_TRUE(c_int.contains(100));
    EXPECT_FALSE(c_int.contains(200));
}

TEST_F(CounterTest, Erase) {
    c_str.add("a", 1);
    c_str.add("b", 2);
    ASSERT_EQ(c_str.size(), 2);

    EXPECT_EQ(c_str.erase("a"), 1); // Erase existing
    EXPECT_EQ(c_str.size(), 1);
    EXPECT_FALSE(c_str.contains("a"));

    EXPECT_EQ(c_str.erase("c"), 0); // Erase non-existing
    EXPECT_EQ(c_str.size(), 1);
}

TEST_F(CounterTest, RemoveDeprecated) {
    c_str.add("a", 1);
    c_str.add("b", 2);
#if defined(__GNUC__) && !defined(__clang__) // Specific warning for GCC
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
    c_str.remove("a");
#if defined(__GNUC__) && !defined(__clang__)
    #pragma GCC diagnostic pop
#elif defined(__clang__)
    #pragma clang diagnostic pop
#endif
    EXPECT_EQ(c_str.size(), 1);
    EXPECT_FALSE(c_str.contains("a"));
}


TEST_F(CounterTest, Clear) {
    c_int.add(1, 5);
    c_int.add(2, 3);
    ASSERT_FALSE(c_int.empty());
    c_int.clear();
    EXPECT_TRUE(c_int.empty());
    EXPECT_EQ(c_int.size(), 0);
    EXPECT_EQ(c_int.count(1), 0); // All counts should be zero
}

TEST_F(CounterTest, SizeAndEmpty) { // Already partially tested
    EXPECT_TRUE(c_int.empty());
    EXPECT_EQ(c_int.size(), 0);
    c_int.add(1);
    EXPECT_FALSE(c_int.empty());
    EXPECT_EQ(c_int.size(), 1);
    c_int.erase(1);
    EXPECT_TRUE(c_int.empty());
    EXPECT_EQ(c_int.size(), 0);
}

// 3. Iterators
TEST_F(CounterTest, IteratorsRangeBasedFor) {
    c_str.add("apple", 2);
    c_str.add("banana", 3);
    std::map<std::string, int> iterated_items;
    for (const auto& pair : c_str) { // Tests begin() and end()
        EXPECT_TRUE(pair.first == "apple" || pair.first == "banana");
        iterated_items[pair.first] = pair.second;
    }
    EXPECT_EQ(iterated_items.size(), 2);
    EXPECT_EQ(iterated_items["apple"], 2);
    EXPECT_EQ(iterated_items["banana"], 3);

    // Const iterators
    const Counter<std::string>& const_c_str = c_str;
    std::map<std::string, int> const_iterated_items;
    for (const auto& pair : const_c_str) { // Tests cbegin() and cend()
        const_iterated_items[pair.first] = pair.second;
    }
    EXPECT_EQ(const_iterated_items, iterated_items);
}

// 4. most_common()
TEST_F(CounterTest, MostCommon) {
    c_str.add("a", 1);
    c_str.add("b", 5);
    c_str.add("c", 3);
    c_str.add("d", 5); // Tie with "b"

    // All items (n=0)
    auto common_all = c_str.most_common();
    ASSERT_EQ(common_all.size(), 4);
    // Order: (b,5), (d,5) (tie broken by key: b < d), (c,3), (a,1)
    EXPECT_EQ(common_all[0].first, "b"); EXPECT_EQ(common_all[0].second, 5);
    EXPECT_EQ(common_all[1].first, "d"); EXPECT_EQ(common_all[1].second, 5);
    EXPECT_EQ(common_all[2].first, "c"); EXPECT_EQ(common_all[2].second, 3);
    EXPECT_EQ(common_all[3].first, "a"); EXPECT_EQ(common_all[3].second, 1);

    // Top 2
    auto common_top2 = c_str.most_common(2);
    ASSERT_EQ(common_top2.size(), 2);
    EXPECT_EQ(common_top2[0].first, "b"); EXPECT_EQ(common_top2[0].second, 5);
    EXPECT_EQ(common_top2[1].first, "d"); EXPECT_EQ(common_top2[1].second, 5);

    // n > size
    auto common_n_gt_size = c_str.most_common(10);
    ASSERT_EQ(common_n_gt_size.size(), 4); // Should return all
    EXPECT_EQ(common_n_gt_size, common_all); // Order should be the same

    // Empty counter
    Counter<int> empty_counter;
    auto common_empty = empty_counter.most_common();
    EXPECT_TRUE(common_empty.empty());
    auto common_empty_n = empty_counter.most_common(5);
    EXPECT_TRUE(common_empty_n.empty());
}

// 5. Arithmetic Operators
TEST_F(CounterTest, OperatorPlusEquals) {
    c_int.add(1, 2); c_int.add(2, 1);
    Counter<int> c2;
    c2.add(2, 3); c2.add(3, 5);

    c_int += c2;
    EXPECT_EQ(c_int.count(1), 2);
    EXPECT_EQ(c_int.count(2), 4);
    EXPECT_EQ(c_int.count(3), 5);
    EXPECT_EQ(c_int.size(), 3);
}

TEST_F(CounterTest, OperatorMinusEquals) {
    c_int.add(1, 10); c_int.add(2, 5); c_int.add(3, 2);
    Counter<int> c2;
    c2.add(1, 3); c2.add(2, 7); c2.add(4, 1); // 2 will be removed

    c_int -= c2;
    EXPECT_EQ(c_int.count(1), 7);
    EXPECT_FALSE(c_int.contains(2)); // Count became 5-7 = -2, removed
    EXPECT_EQ(c_int.count(3), 2);
    EXPECT_FALSE(c_int.contains(4)); // Not affected
    EXPECT_EQ(c_int.size(), 2);
}

TEST_F(CounterTest, OperatorPlus) {
    c_str.add("a", 1); c_str.add("b", 2);
    Counter<std::string> c2;
    c2.add("b", 3); c2.add("c", 4);

    auto result = c_str + c2;
    EXPECT_EQ(result.count("a"), 1);
    EXPECT_EQ(result.count("b"), 5);
    EXPECT_EQ(result.count("c"), 4);
    EXPECT_EQ(result.size(), 3);
    // Originals unchanged
    EXPECT_EQ(c_str.count("b"), 2);
    EXPECT_EQ(c2.count("b"), 3);
}

TEST_F(CounterTest, OperatorMinus) {
    c_str.add("a", 5); c_str.add("b", 3); c_str.add("d",1);
    Counter<std::string> c2;
    c2.add("a", 2); c2.add("b", 5); c2.add("c", 1); // b will be removed

    auto result = c_str - c2;
    EXPECT_EQ(result.count("a"), 3);
    EXPECT_FALSE(result.contains("b"));
    EXPECT_FALSE(result.contains("c"));
    EXPECT_EQ(result.count("d"),1);
    EXPECT_EQ(result.size(), 2);
}

// 6. Comparison Operators
TEST_F(CounterTest, EqualityOperators) {
    c_int.add(1, 1); c_int.add(2, 2);
    Counter<int> c2; c2.add(1, 1); c2.add(2, 2);
    Counter<int> c3; c3.add(1, 1); c3.add(2, 3); // Different count
    Counter<int> c4; c4.add(1, 1); c4.add(3, 2); // Different key

    EXPECT_TRUE(c_int == c2);
    EXPECT_FALSE(c_int == c3);
    EXPECT_FALSE(c_int == c4);

    EXPECT_FALSE(c_int != c2);
    EXPECT_TRUE(c_int != c3);
    EXPECT_TRUE(c_int != c4);

    Counter<int> empty1, empty2;
    EXPECT_TRUE(empty1 == empty2);
    EXPECT_FALSE(empty1 == c_int);
}

// 7. Utility Methods
TEST_F(CounterTest, Total) {
    EXPECT_EQ(c_int.total(), 0);
    c_int.add(1, 5);
    c_int.add(2, 10);
    c_int.add(3, -2); // Manually set negative count (total should reflect this)
    c_int.erase(3);   // Remove the item with negative count for accurate total of positive counts
    c_int.add(4,2);

    // The total() method in typical Python Counter sums positive counts.
    // Let's assume this Counter does the same. If it sums all values, adjust test.
    // Based on typical Counter behavior, only positive counts are stored and summed.
    // If internal map can store negative values via operator[], `total` might behave differently.
    // The provided Counter.h implies positive counts for `add` and `subtract` logic.
    // Let's test based on the assumption it sums counts as stored.

    Counter<int> c;
    c.add(1,5);
    c.add(2,10);
    EXPECT_EQ(c.total(), 15);
    c[3] = -5; // direct manipulation
    EXPECT_EQ(c.total(), 10); // 5 + 10 - 5
    c.add(4,2); // 5 + 10 - 5 + 2
    EXPECT_EQ(c.total(), 12);
}

// 8. Set Operations
TEST_F(CounterTest, Intersection) {
    c_int.add(1, 5); c_int.add(2, 3); c_int.add(3, 1);
    Counter<int> c2;
    c2.add(2, 4); c2.add(3, 0); c2.add(4, 5); c2.add(1,2);

    auto result = c_int.intersection(c2);
    EXPECT_EQ(result.count(1), 2); // min(5, 2)
    EXPECT_EQ(result.count(2), 3); // min(3, 4)
    EXPECT_FALSE(result.contains(3)); // min(1, 0) -> not included
    EXPECT_FALSE(result.contains(4));
    EXPECT_EQ(result.size(), 2);
}

TEST_F(CounterTest, UnionWith) {
    c_int.add(1, 5); c_int.add(2, 3); c_int.add(3, 1);
    Counter<int> c2;
    c2.add(2, 4); c2.add(3, -2); c2.add(4, 5); c2.add(1,2);

    auto result = c_int.union_with(c2);
    EXPECT_EQ(result.count(1), 5); // max(5, 2)
    EXPECT_EQ(result.count(2), 4); // max(3, 4)
    EXPECT_EQ(result.count(3), 1); // max(1, -2 -> effectively 0 for positive counters)
                                   // If negatives are stored, this test needs adjustment based on counter.h
                                   // Assuming union_with prefers positive counts or actual stored values.
                                   // The implementation detail of union_with matters here.
                                   // Based on the example, it seems to take max of actual stored values.
    c2[3] = -2; // ensure it's set
    result = c_int.union_with(c2); // re-evaluate
    EXPECT_EQ(result.count(3), 1); // max(1, -2) = 1

    EXPECT_EQ(result.count(4), 5); // max(0, 5)
    EXPECT_EQ(result.size(), 4);
}


// 9. Filter Operations
TEST_F(CounterTest, PositiveNegativeFilters) {
    c_int.add(1, 3);
    c_int.add(2, 0); // Not added
    c_int.add(3, -2); // Added via subtract, so not present
    c_int[4] = -5;   // Manually set negative
    c_int[5] = 2;
    c_int[6] = 0;    // Manually set zero (should be removed by positive/negative?)

    auto positives = c_int.positive();
    EXPECT_EQ(positives.count(1), 3);
    EXPECT_EQ(positives.count(5), 2);
    EXPECT_FALSE(positives.contains(2));
    EXPECT_FALSE(positives.contains(3));
    EXPECT_FALSE(positives.contains(4));
    EXPECT_FALSE(positives.contains(6)); // Zero counts usually excluded
    EXPECT_EQ(positives.size(), 2);

    auto negatives = c_int.negative();
    EXPECT_EQ(negatives.count(4), -5);
    EXPECT_FALSE(negatives.contains(1));
    EXPECT_FALSE(negatives.contains(5));
    EXPECT_FALSE(negatives.contains(6));
    EXPECT_EQ(negatives.size(), 1);
}

TEST_F(CounterTest, FilterCustomPredicate) {
    c_int.add(1, 1); c_int.add(2, 2); c_int.add(3, 3); c_int.add(4, 4);
    auto evens = c_int.filter([](const int& key, int count) {
        return key % 2 == 0;
    });
    EXPECT_FALSE(evens.contains(1));
    EXPECT_EQ(evens.count(2), 2);
    EXPECT_FALSE(evens.contains(3));
    EXPECT_EQ(evens.count(4), 4);
    EXPECT_EQ(evens.size(), 2);

    auto count_gt_2 = c_int.filter([](const int& /*key*/, int count) {
        return count > 2;
    });
    EXPECT_FALSE(count_gt_2.contains(1));
    EXPECT_FALSE(count_gt_2.contains(2));
    EXPECT_EQ(count_gt_2.count(3), 3);
    EXPECT_EQ(count_gt_2.count(4), 4);
    EXPECT_EQ(count_gt_2.size(), 2);
}

// 10. Custom Types
TEST_F(CounterTest, CustomTypeCounter) {
    Counter<TestPerson, TestPersonHash> person_counter;

    TestPerson alice1("Alice", 25);
    TestPerson bob("Bob", 30);
    TestPerson alice2("Alice", 25); // Same as alice1

    person_counter.add(alice1);
    person_counter.add(bob);
    person_counter.add(alice2); // Should increment count for Alice

    EXPECT_EQ(person_counter.count(TestPerson("Alice", 25)), 2);
    EXPECT_EQ(person_counter.count(bob), 1);
    EXPECT_EQ(person_counter.size(), 2);

    person_counter.subtract(bob);
    EXPECT_FALSE(person_counter.contains(bob));

    // Test most_common with custom type
    person_counter.add(TestPerson("Charlie", 35), 5);
    person_counter.add(TestPerson("Alice", 25), 3); // Alice is now 2+3=5
    // Counts: Alice: 5, Charlie: 5

    auto common_persons = person_counter.most_common();
    ASSERT_EQ(common_persons.size(), 2);
    // Tie-breaking: Alice < Charlie by name
    EXPECT_EQ(common_persons[0].first, TestPerson("Alice", 25));
    EXPECT_EQ(common_persons[0].second, 5);
    EXPECT_EQ(common_persons[1].first, TestPerson("Charlie", 35));
    EXPECT_EQ(common_persons[1].second, 5);
}

// Test for operator[] creating default (0) then allowing modification
TEST_F(CounterTest, OperatorBracketDefaultCreationAndModification) {
    Counter<std::string> c;
    EXPECT_EQ(c["new_item"], 0); // Access creates with count 0
    EXPECT_TRUE(c.contains("new_item"));
    EXPECT_EQ(c.size(), 1);

    c["new_item"] = 10;
    EXPECT_EQ(c["new_item"], 10);
    EXPECT_EQ(c.size(), 1);

    c["another_item"] += 5; // Creates with 0, then adds 5
    EXPECT_EQ(c["another_item"], 5);
    EXPECT_EQ(c.size(), 2);
}
// Note: The GTest main function is usually linked from GTest::gtest_main,
// so it's not needed in the test file itself.
// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }
