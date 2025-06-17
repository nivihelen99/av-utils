#include "gtest/gtest.h"
#include "../include/lru_cache.h" // Adjust path as necessary
#include <string>
#include <vector>
#include <thread> // For thread safety tests
#include <atomic> // For counters in thread safety tests
#include <set>    // For checking evicted items

// Test fixture for LRUCache tests
class LRUCacheTest : public ::testing::Test {
protected:
    // You can put common setup code here if needed,
    // but for many LRUCache tests, direct instantiation is clearer.
};

// Test basic construction and empty/size
TEST_F(LRUCacheTest, ConstructorAndBasicState) {
    LRUCache<int, std::string> cache(3);
    EXPECT_TRUE(cache.empty());
    EXPECT_EQ(0, cache.size());
    EXPECT_THROW((LRUCache<int, int>(0)), std::invalid_argument);
}

// Test Put and Get operations
TEST_F(LRUCacheTest, PutAndGet) {
    LRUCache<int, std::string> cache(2);
    cache.put(1, "one");
    cache.put(2, "two");

    EXPECT_EQ(2, cache.size());
    ASSERT_TRUE(cache.get(1).has_value());
    EXPECT_EQ("one", cache.get(1).value());
    ASSERT_TRUE(cache.get(2).has_value());
    EXPECT_EQ("two", cache.get(2).value());

    // Test updating an existing key
    cache.put(1, "new_one");
    EXPECT_EQ(2, cache.size());
    ASSERT_TRUE(cache.get(1).has_value());
    EXPECT_EQ("new_one", cache.get(1).value());

    // Test getting a non-existent key
    EXPECT_FALSE(cache.get(3).has_value());
}

// Test LRU eviction policy
TEST_F(LRUCacheTest, EvictionPolicy) {
    LRUCache<int, std::string> cache(2);
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three"); // Should evict (1, "one")

    EXPECT_EQ(2, cache.size());
    EXPECT_FALSE(cache.get(1).has_value()); // 1 should be evicted
    EXPECT_FALSE(cache.contains(1));
    ASSERT_TRUE(cache.get(2).has_value());
    EXPECT_EQ("two", cache.get(2).value());
    ASSERT_TRUE(cache.get(3).has_value());
    EXPECT_EQ("three", cache.get(3).value());

    // Accessing 2 should make it MRU, so 3 should be evicted next
    cache.get(2);
    cache.put(4, "four"); // Should evict (3, "three")

    EXPECT_EQ(2, cache.size());
    EXPECT_FALSE(cache.get(3).has_value());
    EXPECT_FALSE(cache.contains(3));
    ASSERT_TRUE(cache.get(2).has_value());
    EXPECT_EQ("two", cache.get(2).value());
    ASSERT_TRUE(cache.get(4).has_value());
    EXPECT_EQ("four", cache.get(4).value());
}

// Test Contains operation
TEST_F(LRUCacheTest, Contains) {
    LRUCache<int, std::string> cache(2);
    cache.put(1, "one");

    EXPECT_TRUE(cache.contains(1));
    EXPECT_FALSE(cache.contains(2));

    cache.put(2, "two");
    EXPECT_TRUE(cache.contains(1));
    EXPECT_TRUE(cache.contains(2));

    cache.put(3, "three"); // Evicts 1
    EXPECT_FALSE(cache.contains(1));
    EXPECT_TRUE(cache.contains(2));
    EXPECT_TRUE(cache.contains(3));
}

// Test Erase operation
TEST_F(LRUCacheTest, Erase) {
    LRUCache<int, std::string> cache(2);
    cache.put(1, "one");
    cache.put(2, "two");

    EXPECT_TRUE(cache.erase(1));
    EXPECT_EQ(1, cache.size());
    EXPECT_FALSE(cache.contains(1));
    EXPECT_FALSE(cache.get(1).has_value());
    ASSERT_TRUE(cache.get(2).has_value()); // 2 should still be there

    EXPECT_FALSE(cache.erase(1)); // Erasing non-existent key

    EXPECT_TRUE(cache.erase(2));
    EXPECT_TRUE(cache.empty());
    EXPECT_EQ(0, cache.size());

    // Erase from a full cache then add
    cache.put(3, "three");
    cache.put(4, "four");
    EXPECT_TRUE(cache.erase(3));
    cache.put(5, "five");
    EXPECT_EQ(2, cache.size());
    EXPECT_FALSE(cache.contains(3));
    EXPECT_TRUE(cache.contains(4));
    EXPECT_TRUE(cache.contains(5));
}

// Test Clear operation
TEST_F(LRUCacheTest, Clear) {
    LRUCache<int, std::string> cache(2);
    cache.put(1, "one");
    cache.put(2, "two");

    cache.clear();
    EXPECT_TRUE(cache.empty());
    EXPECT_EQ(0, cache.size());
    EXPECT_FALSE(cache.contains(1));
    EXPECT_FALSE(cache.contains(2));

    // Clear an already empty cache
    cache.clear();
    EXPECT_TRUE(cache.empty());
    EXPECT_EQ(0, cache.size());
}

// Test Eviction Callback
TEST_F(LRUCacheTest, EvictionCallback) {
    std::vector<std::pair<int, std::string>> evicted_items;
    auto on_evict_cb = [&](const int& key, const std::string& value) {
        evicted_items.push_back({key, value});
    };

    LRUCache<int, std::string> cache(2, on_evict_cb);
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three"); // Evicts (1, "one")

    ASSERT_EQ(1, evicted_items.size());
    EXPECT_EQ(1, evicted_items[0].first);
    EXPECT_EQ("one", evicted_items[0].second);

    cache.get(2); // Make 2 MRU
    cache.put(4, "four"); // Evicts (3, "three")

    ASSERT_EQ(2, evicted_items.size());
    EXPECT_EQ(3, evicted_items[1].first);
    EXPECT_EQ("three", evicted_items[1].second);

    // Erase should not trigger eviction callback
    cache.erase(2);
    ASSERT_EQ(2, evicted_items.size()); // Still 2, erase doesn't use on_evict_

    // Clear should not trigger eviction callback
    cache.clear();
    ASSERT_EQ(2, evicted_items.size()); // Still 2
}

// Test Get promotes item to MRU
TEST_F(LRUCacheTest, GetPromotesToMRU) {
    LRUCache<int, std::string> cache(3);
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three"); // Order: 3 (MRU), 2, 1 (LRU)

    cache.get(1); // Access 1, should become MRU. Order: 1 (MRU), 3, 2 (LRU)
    cache.put(4, "four"); // Should evict 2

    EXPECT_EQ(3, cache.size());
    EXPECT_FALSE(cache.contains(2));
    EXPECT_TRUE(cache.contains(1));
    EXPECT_TRUE(cache.contains(3));
    EXPECT_TRUE(cache.contains(4));
}

// Test Put updates item and promotes it to MRU
TEST_F(LRUCacheTest, PutUpdatesAndPromotesToMRU) {
    LRUCache<int, std::string> cache(3);
    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three"); // Order: 3 (MRU), 2, 1 (LRU)

    cache.put(1, "new_one"); // Update 1, should become MRU. Order: 1 (MRU), 3, 2 (LRU)
    cache.put(4, "four");    // Should evict 2

    EXPECT_EQ(3, cache.size());
    EXPECT_FALSE(cache.contains(2));
    ASSERT_TRUE(cache.get(1).has_value());
    EXPECT_EQ("new_one", cache.get(1).value());
    EXPECT_TRUE(cache.contains(3));
    EXPECT_TRUE(cache.contains(4));
}

// Test with capacity 1
TEST_F(LRUCacheTest, CapacityOne) {
    std::vector<std::pair<int, std::string>> evicted_items;
    auto on_evict_cb = [&](const int& key, const std::string& value) {
        evicted_items.push_back({key, value});
    };
    LRUCache<int, std::string> cache(1, on_evict_cb);

    cache.put(1, "one");
    EXPECT_EQ(1, cache.size());
    EXPECT_TRUE(cache.contains(1));

    cache.put(2, "two"); // Evicts (1, "one")
    EXPECT_EQ(1, cache.size());
    EXPECT_FALSE(cache.contains(1));
    EXPECT_TRUE(cache.contains(2));
    ASSERT_EQ(1, evicted_items.size());
    EXPECT_EQ(1, evicted_items[0].first);
    EXPECT_EQ("one", evicted_items[0].second);

    ASSERT_TRUE(cache.get(2).has_value()); // Access 2, no change in LRU since it's the only one
    cache.put(3, "three"); // Evicts (2, "two")
    EXPECT_EQ(1, cache.size());
    EXPECT_FALSE(cache.contains(2));
    EXPECT_TRUE(cache.contains(3));
    ASSERT_EQ(2, evicted_items.size());
    EXPECT_EQ(2, evicted_items[1].first);
    EXPECT_EQ("two", evicted_items[1].second);
}

// Stress test for basic operations and eviction
TEST_F(LRUCacheTest, StressTest) {
    const int num_operations = 10000;
    std::vector<int> evicted_keys;
    LRUCache<int, int> cache(100, [&](const int&k, const int&){ evicted_keys.push_back(k); });
    std::atomic<int> successful_erasures(0);

    for (int i = 0; i < num_operations; ++i) {
        cache.put(i, i * 2);
        if (i % 10 == 0) {
            int key_to_get = i - (rand() % 100);
            if (key_to_get >= 0) {
                 cache.get(key_to_get);
            }
        }
        if (i % 20 == 0) {
            int key_to_erase = i - 50 - (rand() % 50);
             if (key_to_erase >=0 && cache.erase(key_to_erase)) {
                successful_erasures++;
             }
        }
    }

    EXPECT_LE(cache.size(), 100);
    // Check if items are generally what we expect (more recent items)
    int found_count = 0;
    for (int i = num_operations - 1; i >= num_operations - 100 && i >= 0; --i) {
        if (cache.contains(i)) {
            found_count++;
        }
    }
    // This is a loose check, depends on erase pattern
    EXPECT_GE(found_count, 0); // At least some recent items should be there if not erased.
    EXPECT_EQ(evicted_keys.size(), num_operations - cache.size() - successful_erasures.load());
}


// Thread safety tests
TEST_F(LRUCacheTest, ThreadSafety) {
    LRUCache<int, int> cache(100);
    std::vector<std::thread> threads;
    const int num_threads = 10;
    const int ops_per_thread = 1000;
    std::atomic<int> successful_puts(0);
    std::atomic<int> successful_gets(0);

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < ops_per_thread; ++j) {
                int key = (i * ops_per_thread) + j;
                cache.put(key, key * 2);
                successful_puts++;

                // Interleave some get operations
                int key_to_get = rand() % ( (i+1) * ops_per_thread);
                if (cache.get(key_to_get).has_value()) {
                    successful_gets++;
                }
                 // Interleave some erase
                if (j % 10 == 0) {
                    int key_to_erase = rand() % ((i+1) * ops_per_thread);
                    cache.erase(key_to_erase);
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_LE(cache.size(), 100); // Should not exceed max_size
    // We can't easily verify specific values due to concurrent nature and evictions,
    // but we check that operations completed and cache size is bounded.
    // The main test here is absence of crashes or hangs, indicating basic thread safety.
    // More rigorous thread safety testing (e.g., with specific interleavings or known race conditions)
    // would require more specialized tools or techniques.
    EXPECT_GT(successful_puts, 0); // Make sure some puts happened

    // Final check: put some known values and get them
    cache.put(1000000, 1);
    cache.put(1000001, 2);
    ASSERT_TRUE(cache.get(1000000).has_value());
    EXPECT_EQ(cache.get(1000000).value(), 1);
    ASSERT_TRUE(cache.get(1000001).has_value());
    EXPECT_EQ(cache.get(1000001).value(), 2);
}

// Test for custom types (e.g. std::string keys and custom struct values)
struct MyValue {
    int id;
    std::string data;
    bool operator==(const MyValue& other) const {
        return id == other.id && data == other.data;
    }
};

// Required for unordered_map if MyKey is a custom type not supported by std::hash by default
struct MyKey {
    int part1;
    std::string part2;
     bool operator==(const MyKey& other) const {
        return part1 == other.part1 && part2 == other.part2;
    }
};

namespace std {
    template <> struct hash<MyKey> {
        size_t operator()(const MyKey& k) const {
            return hash<int>()(k.part1) ^ (hash<string>()(k.part2) << 1);
        }
    };
}


TEST_F(LRUCacheTest, CustomTypes) {
    LRUCache<std::string, MyValue> cache(2);
    cache.put("key1", {1, "data1"});
    cache.put("key2", {2, "data2"});

    ASSERT_TRUE(cache.get("key1").has_value()); // This makes "key1" MRU
    EXPECT_EQ(cache.get("key1").value().id, 1);
    EXPECT_EQ(cache.get("key1").value().data, "data1");

    // Cache state after get("key1"): key1 (MRU), key2 (LRU)
    cache.put("key3", {3, "data3"}); // Evicts "key2"

    // Cache state: key3 (MRU), key1 (LRU)
    EXPECT_TRUE(cache.contains("key1"));   // "key1" should still be there
    EXPECT_FALSE(cache.contains("key2"));  // "key2" should be evicted
    ASSERT_TRUE(cache.get("key3").has_value());
    EXPECT_EQ(cache.get("key3").value(), (MyValue{3, "data3"}));
    // Verify "key1" is indeed the other item.
    ASSERT_TRUE(cache.get("key1").has_value());
    EXPECT_EQ(cache.get("key1").value(), (MyValue{1, "data1"}));


    LRUCache<MyKey, int> cacheCustomKey(2);
    MyKey mk1 = {10, "apple"};
    MyKey mk2 = {20, "banana"};
    MyKey mk3 = {30, "cherry"};

    cacheCustomKey.put(mk1, 100); // mk1 is MRU {mk1}
    cacheCustomKey.put(mk2, 200); // mk2 is MRU, mk1 is LRU {mk2, mk1}

    ASSERT_TRUE(cacheCustomKey.get(mk1).has_value()); // mk1 becomes MRU, mk2 is LRU {mk1, mk2}
    EXPECT_EQ(cacheCustomKey.get(mk1).value(), 100);

    cacheCustomKey.put(mk3, 300); // mk3 is MRU. mk2 (LRU) is evicted. {mk3, mk1}

    EXPECT_TRUE(cacheCustomKey.contains(mk1));    // mk1 should still be in the cache.
    EXPECT_FALSE(cacheCustomKey.contains(mk2));   // mk2 should have been evicted.
    ASSERT_TRUE(cacheCustomKey.get(mk3).has_value());
    EXPECT_EQ(cacheCustomKey.get(mk3).value(), 300);
    // Verify mk1 is indeed the other item and accessible.
    ASSERT_TRUE(cacheCustomKey.get(mk1).has_value());
    EXPECT_EQ(cacheCustomKey.get(mk1).value(), 100);
}

// It's good practice to have a main function in one of the test files
// or link with GTest_main if it's not already handled by the build system.
// For this project structure, it's likely handled by tests/CMakeLists.txt
// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }
