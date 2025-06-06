#include "gtest/gtest.h"
#include "skiplist.h" // Assuming skiplist.h is in the root directory
#include <vector>
#include <string>
#include <algorithm> // For std::sort
#include <utility>   // For std::pair
#include <random>    // For StressTest (if enabled)

// --- MyData Struct and Utilities ---
#include <string>    // For std::string
#include <utility>   // For std::move (in constructor)
#include <vector>    // For tests
#include <algorithm> // For std::sort in tests
#include <functional>// For std::less in tests
#include <iomanip>   // For std::fixed, std::setprecision
#include <cmath>     // For std::abs (double comparison)
#include <sstream>   // For std::ostringstream in value_to_log_string

struct MyData; // Forward declare MyData
template<> std::string value_to_log_string<MyData>(const MyData& val); // Forward declare specialization

struct MyData {
    int id;
    std::string name;
    double score;
    bool is_active;

    MyData(int i = 0, std::string n = "", double s = 0.0, bool active = false)
        : id(i), name(std::move(n)), score(s), is_active(active) {}

    bool operator==(const MyData& other) const {
        return id == other.id &&
               name == other.name &&
               (std::abs(score - other.score) < 1e-9) &&
               is_active == other.is_active;
    }

    bool operator!=(const MyData& other) const {
        return !(*this == other);
    }

    bool operator<(const MyData& other) const {
        if (id != other.id) return id < other.id;
        if (name != other.name) return name < other.name;
        if (std::abs(score - other.score) >= 1e-9) { // If scores are significantly different
            return score < other.score;
        }
        // If scores are "equal" or very close, compare by is_active
        return is_active < other.is_active;
    }
};

template<>
std::string value_to_log_string<MyData>(const MyData& val) {
    std::ostringstream oss;
    oss << "MyData(id=" << val.id
        << ", name=\"" << val.name << "\""
        << ", score=" << std::fixed << std::setprecision(2) << val.score
        << ", active=" << (val.is_active ? "true" : "false") << ")";
    return oss.str();
}

namespace std {
    template<>
    struct less<MyData> {
        bool operator()(const MyData& lhs, const MyData& rhs) const {
            return lhs < rhs;
        }
    };
}
// --- End of MyData Struct and Utilities ---

// Helper to compare vector contents ignoring order
template<typename T>
void ExpectVectorsEqualUnordered(std::vector<T> vec1, std::vector<T> vec2) {
    std::sort(vec1.begin(), vec1.end());
    std::sort(vec2.begin(), vec2.end());
    EXPECT_EQ(vec1, vec2);
}

TEST(SkipListTest, EmptyList) {
    SkipList<int> sl;
    EXPECT_EQ(sl.size(), 0);
    EXPECT_FALSE(sl.search(10));
    EXPECT_FALSE(sl.remove(10));
    auto range = sl.rangeQuery(0, 100);
    EXPECT_TRUE(range.empty());
    EXPECT_THROW(sl.kthElement(0), std::out_of_range);

    int count = 0;
    for (int val : sl) {
        count++;
    }
    EXPECT_EQ(count, 0);
}

TEST(SkipListTest, BasicIntOperations) {
    SkipList<int> sl;
    sl.insert(3);
    sl.insert(6);
    sl.insert(1);
    sl.insert(9);
    sl.insert(6); // Duplicate insert

    EXPECT_EQ(sl.size(), 4);
    EXPECT_TRUE(sl.search(3));
    EXPECT_TRUE(sl.search(6));
    EXPECT_TRUE(sl.search(1));
    EXPECT_TRUE(sl.search(9));
    EXPECT_FALSE(sl.search(5));

    EXPECT_TRUE(sl.remove(6));
    EXPECT_FALSE(sl.search(6));
    EXPECT_EQ(sl.size(), 3);

    EXPECT_FALSE(sl.remove(100)); // Remove non-existent
    EXPECT_EQ(sl.size(), 3);
}

// TEST(SkipListTest, BasicIntOperations_MinimalCrashDebug) { // Commented out
//     SkipList<int> sl;
//     std::cout << "Minimal Test: Inserting 3" << std::endl;
//     sl.insert(3);
//     std::cout << "Minimal Test: Searching 3" << std::endl;
//     bool found = sl.search(3);
//     std::cout << "Minimal Test: Searched 3, found=" << found << std::endl;
//     EXPECT_TRUE(found);
// }

TEST(SkipListTest, StringOperations) {
    SkipList<std::string> sl;
    sl.insert("apple");
    sl.insert("banana");
    sl.insert("cherry");
    sl.insert("apple"); // Duplicate

    EXPECT_EQ(sl.size(), 3);
    EXPECT_TRUE(sl.search("apple"));
    EXPECT_TRUE(sl.search("banana"));
    EXPECT_FALSE(sl.search("orange"));

    EXPECT_TRUE(sl.remove("banana"));
    EXPECT_FALSE(sl.search("banana"));
    EXPECT_EQ(sl.size(), 2);
}

TEST(SkipListTest, KthElement) {
    SkipList<int> sl;
    std::vector<int> values = {10, 5, 20, 15, 25, 0};
    for (int v : values) {
        sl.insert(v);
    }
    // Sorted order: 0, 5, 10, 15, 20, 25
    EXPECT_EQ(sl.size(), 6);
    EXPECT_EQ(sl.kthElement(0), 0);
    EXPECT_EQ(sl.kthElement(1), 5);
    EXPECT_EQ(sl.kthElement(3), 15);
    EXPECT_EQ(sl.kthElement(5), 25);

    EXPECT_THROW(sl.kthElement(6), std::out_of_range);
    EXPECT_THROW(sl.kthElement(-1), std::invalid_argument);
}

TEST(SkipListTest, RangeQuery) {
    SkipList<int> sl;
    std::vector<int> values = {10, 5, 20, 15, 25, 0, 30, 35};
    for (int v : values) {
        sl.insert(v);
    }
    // Sorted: 0, 5, 10, 15, 20, 25, 30, 35

    std::vector<int> expected_range1 = {10, 15, 20, 25};
    ExpectVectorsEqualUnordered(sl.rangeQuery(10, 25), expected_range1);

    std::vector<int> expected_range2 = {0, 5};
    ExpectVectorsEqualUnordered(sl.rangeQuery(-5, 7), expected_range2);

    std::vector<int> expected_range3 = {30, 35};
    ExpectVectorsEqualUnordered(sl.rangeQuery(30, 100), expected_range3);

    std::vector<int> expected_range_all = {0, 5, 10, 15, 20, 25, 30, 35};
    ExpectVectorsEqualUnordered(sl.rangeQuery(0, 35), expected_range_all);

    EXPECT_TRUE(sl.rangeQuery(100, 200).empty());
    EXPECT_TRUE(sl.rangeQuery(7, 9).empty());
}

TEST(SkipListTest, Iterators) {
    SkipList<int> sl;
    std::vector<int> values = {10, 5, 20, 15, 2};
    for (int v : values) {
        sl.insert(v);
    }
    // Expected sorted: 2, 5, 10, 15, 20

    std::vector<int> iterated_values;
    for (int val : sl) {
        iterated_values.push_back(val);
    }
    std::vector<int> sorted_values = values;
    std::sort(sorted_values.begin(), sorted_values.end());
    EXPECT_EQ(iterated_values, sorted_values);

    // Const iterator test
    const SkipList<int>& const_sl = sl;
    iterated_values.clear();
    for (int val : const_sl) {
        iterated_values.push_back(val);
    }
    EXPECT_EQ(iterated_values, sorted_values);

    SkipList<int>::iterator it = sl.begin();
    EXPECT_EQ(*it, 2);
    ++it;
    EXPECT_EQ(*it, 5);

    SkipList<int>::const_iterator cit = const_sl.cbegin();
    EXPECT_EQ(*cit, 2);
    ++cit;
    EXPECT_EQ(*cit, 5);
}

TEST(SkipListTest, BulkOperationsInt) {
    SkipList<int> sl;
    std::vector<int> initial_values = {50, 10, 30, 20, 60, 40, 30}; // Unsorted, with duplicate
    sl.insert_bulk(initial_values);

    EXPECT_EQ(sl.size(), 6);
    std::vector<int> expected_after_insert = {10, 20, 30, 40, 50, 60};
    std::vector<int> actual_after_insert;
    for(int val : sl) actual_after_insert.push_back(val);
    EXPECT_EQ(actual_after_insert, expected_after_insert);

    std::vector<int> remove_values = {30, 70, 10, 30, 5};
    size_t removed_count = sl.remove_bulk(remove_values);
    EXPECT_EQ(removed_count, 2);

    EXPECT_EQ(sl.size(), 4);
    std::vector<int> expected_after_remove = {20, 40, 50, 60};
    std::vector<int> actual_after_remove;
    for(int val : sl) actual_after_remove.push_back(val);
    EXPECT_EQ(actual_after_remove, expected_after_remove);

    sl.insert_bulk({});
    EXPECT_EQ(sl.size(), 4);

    sl.remove_bulk({});
    EXPECT_EQ(sl.size(), 4);
}

TEST(SkipListTest, BulkOperationsString) {
    SkipList<std::string> sl;
    std::vector<std::string> initial_values = {"orange", "apple", "pear", "banana", "apple"};
    sl.insert_bulk(initial_values);

    EXPECT_EQ(sl.size(), 4);
    std::vector<std::string> expected_after_insert = {"apple", "banana", "orange", "pear"};
    std::vector<std::string> actual_after_insert;
    for(const auto& val : sl) actual_after_insert.push_back(val);
    EXPECT_EQ(actual_after_insert, expected_after_insert);

    std::vector<std::string> remove_values = {"apple", "grape", "pear", "fig", "apple"};
    size_t removed_count = sl.remove_bulk(remove_values);
    EXPECT_EQ(removed_count, 2);

    EXPECT_EQ(sl.size(), 2);
    std::vector<std::string> expected_after_remove = {"banana", "orange"};
    std::vector<std::string> actual_after_remove;
    for(const auto& val : sl) actual_after_remove.push_back(val);
    EXPECT_EQ(actual_after_remove, expected_after_remove);
}

TEST(SkipListTest, KeyValuePairs) {
    SkipList<std::pair<const int, std::string>> sl;

    sl.insert({10, "apple"});
    sl.insert({5, "banana"});
    sl.insert({20, "cherry"});
    sl.insert({5, "orange"});

    EXPECT_EQ(sl.size(), 3);
    EXPECT_TRUE(sl.search({5, ""}));

    auto val_at_5 = sl.kthElement(0);
    EXPECT_EQ(val_at_5.first, 5);
    EXPECT_EQ(val_at_5.second, "banana");

    auto val_at_10 = sl.kthElement(1);
    EXPECT_EQ(val_at_10.first, 10);
    EXPECT_EQ(val_at_10.second, "apple");

    EXPECT_TRUE(sl.remove({5, ""}));
    EXPECT_FALSE(sl.search({5, ""}));
    EXPECT_EQ(sl.size(), 2);

    std::vector<std::pair<const int, std::string>> range_pairs = sl.rangeQuery({0, ""}, {15, ""});
    ASSERT_EQ(range_pairs.size(), 1);
    EXPECT_EQ(range_pairs[0].first, 10);
    EXPECT_EQ(range_pairs[0].second, "apple");
}

/*
TEST(SkipListTest, StressTest) {
    SkipList<int> sl;
    const int num_operations = 1000;
    std::vector<int> inserted_elements;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 5000);

    for (int i = 0; i < num_operations; ++i) {
        int val = distrib(gen);
        if (sl.search(val)) {
            EXPECT_TRUE(sl.remove(val));
            auto it = std::find(inserted_elements.begin(), inserted_elements.end(), val);
            if (it != inserted_elements.end()) {
                inserted_elements.erase(it);
            }
        } else {
            sl.insert(val);
            inserted_elements.push_back(val);
        }
    }

    std::sort(inserted_elements.begin(), inserted_elements.end());
    EXPECT_EQ(sl.size(), inserted_elements.size());
    std::vector<int> list_elements;
    for (int v : sl) {
        list_elements.push_back(v);
    }
    EXPECT_EQ(list_elements, inserted_elements);
}
*/

TEST(SkipListTest, CustomStructOperations) {
    SkipList<std::pair<const int, MyData>> sl;

    MyData d1_orig(1, "Alice", 95.01, true);
    MyData d2_orig(2, "Bob", 88.02, false);
    MyData d3_orig(3, "Charlie", 92.53, true);

    sl.insert({d1_orig.id, d1_orig});
    sl.insert({d2_orig.id, d2_orig});
    EXPECT_EQ(sl.size(), 2);
    sl.insert({d3_orig.id, d3_orig});
    EXPECT_EQ(sl.size(), 3);

    EXPECT_TRUE(sl.search({d1_orig.id, MyData()}));
    EXPECT_FALSE(sl.search({100, MyData()}));

    auto it_d1 = sl.find(d1_orig.id);
    ASSERT_NE(it_d1, sl.end());
    EXPECT_EQ(it_d1->first, d1_orig.id);
    EXPECT_EQ(it_d1->second, d1_orig);

    MyData d1_modified = d1_orig;
    d1_modified.name = "Alicia";
    d1_modified.score = 96.04;
    it_d1->second = d1_modified;

    auto it_d1_check = sl.find(d1_orig.id);
    ASSERT_NE(it_d1_check, sl.end());
    EXPECT_EQ(it_d1_check->second, d1_modified);

    MyData d2_updated(d2_orig.id, "Robert", 89.05, true);
    auto res_assign = sl.insert_or_assign({d2_updated.id, d2_updated});
    EXPECT_FALSE(res_assign.second);
    ASSERT_NE(res_assign.first, sl.end());
    EXPECT_EQ(res_assign.first->second, d2_updated);
    EXPECT_EQ(sl.size(), 3);

    MyData d4_orig(4, "David", 77.06, false);
    auto res_insert = sl.insert_or_assign({d4_orig.id, d4_orig});
    EXPECT_TRUE(res_insert.second);
    ASSERT_NE(res_insert.first, sl.end());
    EXPECT_EQ(res_insert.first->second, d4_orig);
    EXPECT_EQ(sl.size(), 4);

    auto range_res_vec = sl.rangeQuery({d1_orig.id, MyData()}, {d3_orig.id, MyData()});

    std::vector<std::pair<const int, MyData>> expected_range;
    expected_range.push_back({d1_modified.id, d1_modified});
    expected_range.push_back({d2_updated.id, d2_updated});
    expected_range.push_back({d3_orig.id, d3_orig});

    ASSERT_EQ(range_res_vec.size(), expected_range.size());
    for(size_t i=0; i < range_res_vec.size(); ++i) {
        EXPECT_EQ(range_res_vec[i].first, expected_range[i].first);
        EXPECT_EQ(range_res_vec[i].second, expected_range[i].second);
    }

    EXPECT_TRUE(sl.remove({d1_modified.id, MyData()}));
    EXPECT_EQ(sl.size(), 3);
    EXPECT_FALSE(sl.search({d1_modified.id, MyData()}));

    std::vector<int> keys_iterated;
    std::vector<MyData> values_iterated;
    for (const auto& entry : sl) {
        keys_iterated.push_back(entry.first);
        values_iterated.push_back(entry.second);
    }
    std::vector<int> expected_keys = {d2_updated.id, d3_orig.id, d4_orig.id};
    std::vector<MyData> expected_values = {d2_updated, d3_orig, d4_orig};

    EXPECT_EQ(keys_iterated, expected_keys); // Relies on iteration order being key order
    ASSERT_EQ(values_iterated.size(), expected_values.size());
    for(size_t i=0; i < values_iterated.size(); ++i) {
        EXPECT_EQ(values_iterated[i], expected_values[i]);
    }

    sl.clear();
    EXPECT_EQ(sl.size(), 0);
    EXPECT_TRUE(sl.begin() == sl.end());
    auto it_after_clear = sl.find(d4_orig.id);
    EXPECT_EQ(it_after_clear, sl.end());
}
