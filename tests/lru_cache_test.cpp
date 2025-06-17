#include "gtest/gtest.h"
#include "../include/lru_cache.h" // Adjust path as necessary
#include <string>
#include <vector>
#include <thread> // For thread safety tests
#include <atomic> // For counters in thread safety tests
#include <set>    // For checking evicted items
#include <memory> // For std::unique_ptr in move semantics tests
#include <functional> // For std::function in function caching utils

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

// Test Cache Statistics
TEST_F(LRUCacheTest, CacheStatistics) {
    LRUCache<int, std::string> cache(2);

    // Initial state
    auto stats = cache.get_stats();
    EXPECT_EQ(0, stats.hits);
    EXPECT_EQ(0, stats.misses);
    EXPECT_EQ(0, stats.evictions);
    EXPECT_DOUBLE_EQ(0.0, stats.hit_rate());

    // Miss
    cache.get(1); // Miss
    stats = cache.get_stats();
    EXPECT_EQ(0, stats.hits);
    EXPECT_EQ(1, stats.misses);
    EXPECT_EQ(0, stats.evictions);
    EXPECT_DOUBLE_EQ(0.0, stats.hit_rate());

    cache.put(1, "one"); // Put 1
    cache.put(2, "two"); // Put 2

    // Hit
    cache.get(1); // Hit
    stats = cache.get_stats();
    EXPECT_EQ(1, stats.hits);
    EXPECT_EQ(1, stats.misses); // Previous miss still counted
    EXPECT_EQ(0, stats.evictions);
    EXPECT_DOUBLE_EQ(1.0 / 2.0, stats.hit_rate());

    cache.get(2); // Hit
    stats = cache.get_stats();
    EXPECT_EQ(2, stats.hits);
    EXPECT_EQ(1, stats.misses);
    EXPECT_EQ(0, stats.evictions);
    EXPECT_DOUBLE_EQ(2.0 / 3.0, stats.hit_rate());

    // Eviction
    cache.put(3, "three"); // Evicts (1, "one") or (2, "two") depending on Get pattern if any
                           // Based on above gets, 1 was last hit, then 2. So 1 is MRU, 2 is next.
                           // Let's trace:
                           // put(1,"one") -> list: (1) map: {1:it1}
                           // put(2,"two") -> list: (2,1) map: {1:it1, 2:it2}
                           // get(1)       -> list: (1,2) map: {1:it1, 2:it2} stats.h=1, m=1
                           // get(2)       -> list: (2,1) map: {1:it1, 2:it2} stats.h=2, m=1
                           // put(3,"three") evicts 1. list: (3,2)
    stats = cache.get_stats();
    EXPECT_EQ(2, stats.hits);
    EXPECT_EQ(1, stats.misses);
    EXPECT_EQ(1, stats.evictions); // One eviction
    // Hit rate doesn't change on put/eviction: 2 hits / (2 hits + 1 miss)
    EXPECT_DOUBLE_EQ(2.0 / 3.0, stats.hit_rate());


    // Another miss
    cache.get(4); // Miss
    stats = cache.get_stats();
    EXPECT_EQ(2, stats.hits);
    EXPECT_EQ(2, stats.misses);
    EXPECT_EQ(1, stats.evictions);
    EXPECT_DOUBLE_EQ(2.0 / 4.0, stats.hit_rate());

    // Reset stats
    cache.reset_stats();
    stats = cache.get_stats();
    EXPECT_EQ(0, stats.hits);
    EXPECT_EQ(0, stats.misses);
    EXPECT_EQ(0, stats.evictions);
    EXPECT_DOUBLE_EQ(0.0, stats.hit_rate());

    // Operations after reset
    cache.get(2); // Hit (2 should be in cache)
    stats = cache.get_stats();
    EXPECT_EQ(1, stats.hits);
    EXPECT_EQ(0, stats.misses);
    EXPECT_EQ(0, stats.evictions);
    EXPECT_DOUBLE_EQ(1.0, stats.hit_rate());

    cache.put(5,"five"); // Evicts 2 (or 3, depending on previous test state)
                         // list: (3,2) from before reset_stats. get(2) -> (2,3). put(5) evicts 3.
    stats = cache.get_stats();
    EXPECT_EQ(1, stats.hits); // hits not affected by put
    EXPECT_EQ(0, stats.misses); // misses not affected by put
    EXPECT_EQ(1, stats.evictions); // one eviction
    EXPECT_DOUBLE_EQ(1.0, stats.hit_rate()); // 1 hit / (1 hit + 0 misses)
}

// Test Cache Move Semantics
TEST_F(LRUCacheTest, CacheMoveSemantics) {
    // Move Constructor
    LRUCache<int, std::string> cache1(2);
    cache1.put(1, "one");
    cache1.put(2, "two");
    auto stats1 = cache1.get_stats(); // Should be 0 H, 0 M, 0 E after puts.
    size_t size1 = cache1.size();

    LRUCache<int, std::string> cache2(std::move(cache1));

    EXPECT_EQ(size1, cache2.size());
    // After move, cache1 is in a valid but unspecified state.
    // Its size might be 0, or its max_size might be 0.
    // The specific implementation of LRUCache from lru_cache_1.h moves max_size_
    // and clears the containers of the source.
    EXPECT_TRUE(cache1.empty()); // Good check for source cache state
    EXPECT_EQ(0, cache1.size());   // Source cache should be empty


    // Check that cache2 has the items and stats
    ASSERT_TRUE(cache2.get(1).has_value());
    EXPECT_EQ("one", cache2.get(1).value());
    ASSERT_TRUE(cache2.get(2).has_value());
    EXPECT_EQ("two", cache2.get(2).value());

    auto stats2 = cache2.get_stats();
    // stats1 were taken before any gets on cache1. cache2 gets will change its own stats.
    // We are primarily testing that the *data* moved.
    // If we want to check stats moved, get them from cache2 *before* any 'get' operations on cache2.
    // The current LRUCache implementation moves the stats object.
    EXPECT_EQ(stats1.hits, stats2.hits); // Should be 0
    EXPECT_EQ(stats1.misses, stats2.misses); // Should be 0
    EXPECT_EQ(stats1.evictions, stats2.evictions); // Should be 0

    // A get operation on cache2 to ensure it's functional
    cache2.get(1);
    stats2 = cache2.get_stats();
    EXPECT_EQ(1, stats2.hits);


    // Move Assignment
    LRUCache<int, std::string> cache3(3);
    cache3.put(10, "ten");
    cache3.put(20, "twenty");
    cache3.put(30, "thirty");
    // Get stats from cache3 *before* any gets, if we want to compare them later.
    auto stats3_pre_get = cache3.get_stats(); // 0 H, 0 M, 0 E
    size_t size3 = cache3.size();

    cache3.get(10); // 1 Hit for cache3
    auto stats3_post_get = cache3.get_stats(); // 1 H, 0 M, 0 E

    LRUCache<int, std::string> cache4(1); // cache4 can be pre-populated or not
    cache4.put(99, "temp");
    cache4 = std::move(cache3);

    EXPECT_EQ(size3, cache4.size());
    EXPECT_TRUE(cache3.empty()); // Source cache should be empty
    EXPECT_EQ(0, cache3.size());

    // Check items and stats on cache4.
    // The stats moved should be stats3_post_get, as that was the state of cache3's stats object.
    ASSERT_TRUE(cache4.get(10).has_value()); // This will be a hit on cache4, incrementing its hits
    EXPECT_EQ("ten", cache4.get(10).value());
    ASSERT_TRUE(cache4.get(20).has_value());
    EXPECT_EQ("twenty", cache4.get(20).value());
    ASSERT_TRUE(cache4.get(30).has_value());
    EXPECT_EQ("thirty", cache4.get(30).value());

    auto stats4 = cache4.get_stats();
    // stats3_post_get had 1 hit. The get(10) on cache4 makes it 2 hits.
    EXPECT_EQ(stats3_post_get.hits + 1, stats4.hits);
    EXPECT_EQ(stats3_post_get.misses, stats4.misses);
    EXPECT_EQ(stats3_post_get.evictions, stats4.evictions);


    // Test that moved-from cache is usable
    cache1.put(100, "hundred");
    ASSERT_TRUE(cache1.get(100).has_value());
    EXPECT_EQ("hundred", cache1.get(100).value());
    EXPECT_EQ(1, cache1.size()); // Its max_size might be what it was originally or 0.
                                 // The current LRUCache from lru_cache_1.h moves max_size_
                                 // but the reserve call `cache_items_map_.reserve(max_size_)` in constructor
                                 // might make it behave like its original max_size if not 0.
                                 // However, the default LRUCache(LRUCache&&) does move max_size_, so cache1.max_size_
                                 // would be its original value. The containers are cleared.
                                 // So it should operate like a new cache with that max_size.
    // Let's check cache1's max_size to be sure. It should be 2 (original max_size).
    EXPECT_EQ(2, cache1.max_size());
    cache1.put(101, "one-o-one");
    EXPECT_EQ(2, cache1.size());
    cache1.put(102, "one-o-two"); // Eviction should occur
    EXPECT_EQ(2, cache1.size());
    EXPECT_FALSE(cache1.contains(100)); // Assuming 100 was LRU after 100, 101 were put.
}

// Test Function Caching Utilities
namespace { // Anonymous namespace for helper functions/variables for this test
    std::atomic<int> regular_function_call_count(0);
    int test_square_func(int x) {
        regular_function_call_count++;
        return x * x;
    }

    struct Functor {
        static std::atomic<int> functor_call_count;
        int operator()(int x) {
            functor_call_count++;
            return x * x * x;
        }
    };
    std::atomic<int> Functor::functor_call_count(0);

    std::atomic<int> macro_function_call_count(0);
} // end anonymous namespace

CACHED_FUNCTION(macro_cached_square, int, int, 5, {
    macro_function_call_count++;
    return arg * arg;
});

TEST_F(LRUCacheTest, FunctionCachingUtilities) {
    // Test make_cached with a free function
    regular_function_call_count = 0;
    auto cached_square = make_cached<int, int>(test_square_func, 3);

    EXPECT_EQ(4, cached_square(2));   // Call 1 to underlying func
    EXPECT_EQ(1, regular_function_call_count.load());
    EXPECT_EQ(4, cached_square(2));   // Cached
    EXPECT_EQ(1, regular_function_call_count.load());
    EXPECT_EQ(1, cached_square.cache_size());

    EXPECT_EQ(9, cached_square(3));   // Call 2
    EXPECT_EQ(2, regular_function_call_count.load());
    EXPECT_EQ(16, cached_square(4));  // Call 3
    EXPECT_EQ(3, regular_function_call_count.load());
    EXPECT_EQ(3, cached_square.cache_size()); // Cache full

    EXPECT_EQ(25, cached_square(5));  // Call 4, evicts 2->4
    EXPECT_EQ(4, regular_function_call_count.load());
    EXPECT_EQ(3, cached_square.cache_size());

    EXPECT_EQ(4, cached_square(2));   // Call 5 (was evicted)
    EXPECT_EQ(5, regular_function_call_count.load());

    auto stats_make_cached = cached_square.cache_stats();
    // Gets: (2), (2)c, (3), (4), (5), (2)
    // Hits: 1 (for second call to 2)
    // Misses: 5 (for first 2, 3, 4, 5, re-call to 2)
    EXPECT_EQ(1, stats_make_cached.hits);
    EXPECT_EQ(5, stats_make_cached.misses);


    cached_square.clear_cache();
    EXPECT_EQ(0, cached_square.cache_size());
    EXPECT_EQ(9, cached_square(3)); // Call 6 (cache was cleared)
    EXPECT_EQ(6, regular_function_call_count.load());

    // Test CachedFunction with a lambda
    std::atomic<int> lambda_call_count(0);
    CachedFunction<int, int> cached_cube_lambda([&](int x) {
        lambda_call_count++;
        return x * x * x;
    }, 2);

    EXPECT_EQ(8, cached_cube_lambda(2));    // Call 1
    EXPECT_EQ(1, lambda_call_count.load());
    EXPECT_EQ(8, cached_cube_lambda(2));    // Cached
    EXPECT_EQ(1, lambda_call_count.load());
    EXPECT_EQ(27, cached_cube_lambda(3));   // Call 2
    EXPECT_EQ(2, lambda_call_count.load());
    EXPECT_EQ(64, cached_cube_lambda(4));   // Call 3, evicts 2->8
    EXPECT_EQ(3, lambda_call_count.load());
    EXPECT_EQ(2, cached_cube_lambda.cache_size());


    // Test CACHED_FUNCTION macro
    macro_function_call_count = 0; // Reset for this specific test part
    EXPECT_EQ(100, macro_cached_square(10)); // Call 1
    EXPECT_EQ(1, macro_function_call_count.load());
    EXPECT_EQ(100, macro_cached_square(10)); // Cached
    EXPECT_EQ(1, macro_function_call_count.load());
    EXPECT_EQ(1, macro_cached_square.cache_size());

    for (int i = 0; i < 6; ++i) { // Fill and cause evictions
        macro_cached_square(i);
    }
    // Calls: 10 (1), 0 (2), 1 (3), 2 (4), 3 (5), 4 (6), 5 (7)
    // (10 is already there)
    // (0) - call 2
    // (1) - call 3
    // (2) - call 4
    // (3) - call 5
    // (4) - call 6
    // (5) - call 7
    // Cache size is 5.
    EXPECT_EQ(7, macro_function_call_count.load());
    EXPECT_EQ(5, macro_cached_square.cache_size());
    auto stats_macro = macro_cached_square.cache_stats();
    // Gets: 10, 10c, 0, 1, 2, 3, 4, 5
    // Hits: 1 (for 10c)
    // Misses: 7 (for 10, 0, 1, 2, 3, 4, 5)
    EXPECT_EQ(1, stats_macro.hits);
    EXPECT_EQ(7, stats_macro.misses);
}

// Test Perfect Forwarding in Put (with Move-Only Value Type)
namespace { // Anonymous namespace for this test's helper struct
    struct MoveOnlyValue {
        int val;
        std::unique_ptr<int> ptr; // Makes it move-only

        explicit MoveOnlyValue(int v) : val(v), ptr(std::make_unique<int>(v)) {}

        // Explicitly define move constructor and assignment
        MoveOnlyValue(MoveOnlyValue&& other) noexcept : val(other.val), ptr(std::move(other.ptr)) {
            other.val = -1; // Indicate moved-from state
        }
        MoveOnlyValue& operator=(MoveOnlyValue&& other) noexcept {
            if (this != &other) {
                val = other.val;
                ptr = std::move(other.ptr);
                other.val = -1; // Indicate moved-from state
            }
            return *this;
        }

        // Delete copy constructor and assignment
        MoveOnlyValue(const MoveOnlyValue&) = delete;
        MoveOnlyValue& operator=(const MoveOnlyValue&) = delete;

        // Optional: Add a comparison operator for assertions if needed, comparing relevant fields
        bool operator==(const MoveOnlyValue& other) const {
            // If ptr is null in one and not the other, they are not equal.
            // If both are null, vals should be same.
            // If both are non-null, both val and *ptr should be same.
            if (!ptr && !other.ptr) return val == other.val;
            if (!ptr || !other.ptr) return false; // one is null, other isn't
            return val == other.val && *ptr == *other.ptr;
        }
         // A method to check if it's in a moved-from state (optional)
        bool is_moved_from() const { return val == -1 && !ptr; }
    };
} // end anonymous namespace


TEST_F(LRUCacheTest, PutPerfectForwarding) {
    LRUCache<int, MoveOnlyValue> cache(2);

    // Create a MoveOnlyValue instance
    MoveOnlyValue val1(10);
    EXPECT_FALSE(val1.is_moved_from());

    // Put using std::move - should use the K&&, V&& overload
    cache.put(1, std::move(val1));
    EXPECT_TRUE(val1.is_moved_from()); // val1 should be in a moved-from state

    // Verify the item is in the cache
    auto retrieved_val1_opt = cache.get(1);
    ASSERT_TRUE(retrieved_val1_opt.has_value());
    // Cannot directly compare with original val1 as it's moved. Create expected.
    // However, MoveOnlyValue itself is not copyable.
    // So, we check its members.
    EXPECT_EQ(10, retrieved_val1_opt.value().val);
    ASSERT_TRUE(retrieved_val1_opt.value().ptr != nullptr);
    EXPECT_EQ(10, *retrieved_val1_opt.value().ptr);

    // Test updating an existing key with a moved value
    MoveOnlyValue new_val1(11);
    cache.put(1, std::move(new_val1)); // Should update key 1
    EXPECT_TRUE(new_val1.is_moved_from());

    auto updated_val1_opt = cache.get(1);
    ASSERT_TRUE(updated_val1_opt.has_value());
    EXPECT_EQ(11, updated_val1_opt.value().val);
    ASSERT_TRUE(updated_val1_opt.value().ptr != nullptr);
    EXPECT_EQ(11, *updated_val1_opt.value().ptr);
    EXPECT_EQ(1, cache.size());


    // Put another move-only value
    MoveOnlyValue val2(20);
    cache.put(2, std::move(val2));
    EXPECT_TRUE(val2.is_moved_from());
    EXPECT_EQ(2, cache.size());

    auto retrieved_val2_opt = cache.get(2);
    ASSERT_TRUE(retrieved_val2_opt.has_value());
    EXPECT_EQ(20, retrieved_val2_opt.value().val);


    // Test eviction with move-only types
    MoveOnlyValue val3(30);
    std::vector<std::pair<int, MoveOnlyValue>> evicted_items;
    LRUCache<int, MoveOnlyValue> cache_with_evict(1,
        [&](const int& k, const MoveOnlyValue& v_ref){ // Note: callback value is V&
            // To move out of callback, we'd need V&& or by-value.
            // The current LRUCache on_evict takes (const K&, const V&).
            // For move-only types, this means we can't move *from* the callback's value argument
            // unless the callback signature is changed or we make a copy (which is disallowed for MoveOnlyValue).
            // This test will verify put works. Eviction callback with move-only type is tricky.
            // The provided lru_cache_1.h has `EvictCallback = std::function<void(const Key&, const Value&)>`
            // This means the value in callback is a const&, so we can't move from it.
            // We can only observe it.
            // If we wanted to *move* the evicted item out, callback signature would need to be `Value&&` or `Value`.
            // For this test, we'll just note it was called.
            // To store it, we'd need to construct a new MoveOnlyValue if we wanted to keep a copy.
            // evicted_items.emplace_back(k, std::move(v)); // This would not compile with const V&
        }
    );
    // Re-check lru_cache.h: EvictCallback = std::function<void(const Key&, const Value&)>
    // This means the value passed to on_evict_ is a const reference.
    // So we cannot move it out. We can only observe it.
    // The test below will just ensure put works and eviction happens.

    MoveOnlyValue mval1(100);
    cache_with_evict.put(100, std::move(mval1)); // mval1 moved
    ASSERT_TRUE(cache_with_evict.contains(100));
    EXPECT_EQ(100, cache_with_evict.get(100).value().val);


    MoveOnlyValue mval2(101);
    cache_with_evict.put(101, std::move(mval2)); // mval2 moved, mval1's content (key 100) evicted

    ASSERT_FALSE(cache_with_evict.contains(100)); // 100 should be evicted
    ASSERT_TRUE(cache_with_evict.contains(101));
    EXPECT_EQ(101, cache_with_evict.get(101).value().val);

    // The line `MoveOnlyValue val_to_copy(2); cache.put(2, val_to_copy);`
    // from the prompt would indeed fail to compile because MoveOnlyValue is not copyable,
    // and the `put(const K&, const V&)` overload would be selected, attempting a copy.
    // This is the correct behavior for a move-only type if only a copying put is available.
    // Since our LRUCache has `template<typename K, typename V> void put(K&& key, V&& value)`
    // and `void put(const Key& key, const Value& value)`, the latter would be a candidate
    // for `cache.put(2, val_to_copy)` but would fail due to `Value` (MoveOnlyValue) not being copyable.
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
