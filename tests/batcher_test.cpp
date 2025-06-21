#include "gtest/gtest.h"
#include "../include/batcher.h" // Adjust path if necessary
#include <vector>
#include <list>
#include <deque>
#include <string>
#include <numeric> // For std::iota

// Helper function to convert a batch (which is a vector) to a string for easy comparison
template<typename T>
std::string batch_to_string(const std::vector<T>& batch) {
    std::string s = "[";
    for (size_t i = 0; i < batch.size(); ++i) {
        s += std::to_string(batch[i]);
        if (i < batch.size() - 1) s += ",";
    }
    s += "]";
    return s;
}

// Specialization for std::string to avoid nested std::to_string
template<>
std::string batch_to_string<std::string>(const std::vector<std::string>& batch) {
    std::string s = "[";
    for (size_t i = 0; i < batch.size(); ++i) {
        s += "\"" + batch[i] + "\"";
        if (i < batch.size() - 1) s += ",";
    }
    s += "]";
    return s;
}


TEST(BatcherTest, BasicVector) {
    std::vector<int> values = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    size_t chunk_size = 3;
    auto batches = batcher(values, chunk_size);

    std::vector<std::vector<int>> expected_batches = {
        {1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {10}
    };

    size_t i = 0;
    for (const auto& batch : batches) {
        ASSERT_LT(i, expected_batches.size());
        EXPECT_EQ(batch, expected_batches[i]);
        i++;
    }
    EXPECT_EQ(i, expected_batches.size());
}

TEST(BatcherTest, VectorEmpty) {
    std::vector<int> values;
    size_t chunk_size = 3;
    auto batches = batcher(values, chunk_size);

    int count = 0;
    for (const auto& batch : batches) {
        (void)batch; // Suppress unused variable warning
        count++;
    }
    EXPECT_EQ(count, 0);
    EXPECT_TRUE(batches.empty());
    EXPECT_EQ(batches.size(), 0);
}

TEST(BatcherTest, VectorSingleElementChunkLarger) {
    std::vector<int> values = {42};
    size_t chunk_size = 3;
    auto batches = batcher(values, chunk_size);

    std::vector<std::vector<int>> expected_batches = {{42}};
    size_t i = 0;
    for (const auto& batch : batches) {
        ASSERT_LT(i, expected_batches.size());
        EXPECT_EQ(batch, expected_batches[i]);
        i++;
    }
    EXPECT_EQ(i, expected_batches.size());
    EXPECT_FALSE(batches.empty());
    EXPECT_EQ(batches.size(), 1);
}

TEST(BatcherTest, VectorSingleElementChunkSmaller) {
    std::vector<int> values = {42};
    size_t chunk_size = 1;
    auto batches = batcher(values, chunk_size);

    std::vector<std::vector<int>> expected_batches = {{42}};
    size_t i = 0;
    for (const auto& batch : batches) {
        ASSERT_LT(i, expected_batches.size());
        EXPECT_EQ(batch, expected_batches[i]);
        i++;
    }
    EXPECT_EQ(i, expected_batches.size());
    EXPECT_EQ(batches.size(), 1);
}


TEST(BatcherTest, VectorExactDivision) {
    std::vector<int> values = {1, 2, 3, 4, 5, 6};
    size_t chunk_size = 2;
    auto batches = batcher(values, chunk_size);

    std::vector<std::vector<int>> expected_batches = {
        {1, 2}, {3, 4}, {5, 6}
    };

    size_t i = 0;
    for (const auto& batch : batches) {
        ASSERT_LT(i, expected_batches.size());
        EXPECT_EQ(batch, expected_batches[i]);
        i++;
    }
    EXPECT_EQ(i, expected_batches.size());
    EXPECT_EQ(batches.size(), 3);
}

TEST(BatcherTest, VectorChunkSizeOne) {
    std::vector<int> values = {1, 2, 3};
    size_t chunk_size = 1;
    auto batches = batcher(values, chunk_size);

    std::vector<std::vector<int>> expected_batches = {{1}, {2}, {3}};
    size_t i = 0;
    for (const auto& batch : batches) {
        ASSERT_LT(i, expected_batches.size());
        EXPECT_EQ(batch, expected_batches[i]);
        i++;
    }
    EXPECT_EQ(i, expected_batches.size());
    EXPECT_EQ(batches.size(), 3);
}

TEST(BatcherTest, VectorChunkSizeEqualToSize) {
    std::vector<int> values = {1, 2, 3, 4, 5};
    size_t chunk_size = values.size();
    auto batches = batcher(values, chunk_size);

    std::vector<std::vector<int>> expected_batches = {{1, 2, 3, 4, 5}};
    size_t i = 0;
    for (const auto& batch : batches) {
        ASSERT_LT(i, expected_batches.size());
        EXPECT_EQ(batch, expected_batches[i]);
        i++;
    }
    EXPECT_EQ(i, expected_batches.size());
    EXPECT_EQ(batches.size(), 1);
}

TEST(BatcherTest, VectorChunkSizeLargerThanSize) {
    std::vector<int> values = {1, 2, 3};
    size_t chunk_size = 5;
    auto batches = batcher(values, chunk_size);

    std::vector<std::vector<int>> expected_batches = {{1, 2, 3}};
    size_t i = 0;
    for (const auto& batch : batches) {
        ASSERT_LT(i, expected_batches.size());
        EXPECT_EQ(batch, expected_batches[i]);
        i++;
    }
    EXPECT_EQ(i, expected_batches.size());
    EXPECT_EQ(batches.size(), 1);
}

TEST(BatcherTest, ListStrings) {
    std::list<std::string> values = {"a", "b", "c", "d", "e"};
    size_t chunk_size = 2;
    auto batches = batcher(values, chunk_size);

    std::vector<std::vector<std::string>> expected_batches = {
        {"a", "b"}, {"c", "d"}, {"e"}
    };

    size_t i = 0;
    for (const auto& batch : batches) {
        ASSERT_LT(i, expected_batches.size());
        EXPECT_EQ(batch, expected_batches[i]);
        i++;
    }
    EXPECT_EQ(i, expected_batches.size());
    EXPECT_EQ(batches.size(), 3);
}

TEST(BatcherTest, DequeChars) {
    std::deque<char> values = {'x', 'y', 'z', 'w', 'v', 'u'};
    size_t chunk_size = 4;
    auto batches = batcher(values, chunk_size);

    // Note: Batch elements are vectors of chars.
    // For comparison, it's easier to compare with vector<char>
    std::vector<std::vector<char>> expected_batches = {
        {'x', 'y', 'z', 'w'}, {'v', 'u'}
    };

    size_t i = 0;
    for (const auto& batch : batches) { // batch is std::vector<char>
        ASSERT_LT(i, expected_batches.size());
        EXPECT_EQ(batch, expected_batches[i]);
        i++;
    }
    EXPECT_EQ(i, expected_batches.size());
    EXPECT_EQ(batches.size(), 2);
}

TEST(BatcherTest, ConstVector) {
    const std::vector<int> values = {10, 20, 30, 40, 50};
    size_t chunk_size = 2;
    auto batches = batcher(values, chunk_size); // Should use BatchView<const Container>

    std::vector<std::vector<int>> expected_batches = {
        {10, 20}, {30, 40}, {50}
    };

    size_t i = 0;
    for (const auto& batch : batches) {
        ASSERT_LT(i, expected_batches.size());
        EXPECT_EQ(batch, expected_batches[i]);
        i++;
    }
    EXPECT_EQ(i, expected_batches.size());
    EXPECT_EQ(batches.size(), 3);
    EXPECT_EQ(batches.chunk_size(), chunk_size);
}

TEST(BatcherTest, BatchViewInfo) {
    std::vector<int> data(11);
    std::iota(data.begin(), data.end(), 1); // 1, 2, ..., 11
    size_t chunk_size = 4;
    auto batch_view = batcher(data, chunk_size);

    EXPECT_EQ(batch_view.chunk_size(), chunk_size);
    EXPECT_FALSE(batch_view.empty());
    // 11 elements, chunk size 4 => ceil(11/4) = ceil(2.75) = 3 batches
    EXPECT_EQ(batch_view.size(), 3);

    std::vector<int> empty_data;
    auto empty_batch_view = batcher(empty_data, chunk_size);
    EXPECT_TRUE(empty_batch_view.empty());
    EXPECT_EQ(empty_batch_view.size(), 0);
}

TEST(BatcherTest, IteratorFunctionality) {
    std::vector<int> values = {1, 2, 3, 4, 5};
    size_t chunk_size = 2;
    auto batches = batcher(values, chunk_size);

    auto it = batches.begin();
    ASSERT_NE(it, batches.end());
    EXPECT_EQ(*it, std::vector<int>({1, 2}));

    ++it;
    ASSERT_NE(it, batches.end());
    EXPECT_EQ(*it, std::vector<int>({3, 4}));

    auto it_copy = it++; // Test post-increment
    ASSERT_NE(it, batches.end());
    EXPECT_EQ(*it, std::vector<int>({5}));
    EXPECT_EQ(*it_copy, std::vector<int>({3, 4})); // it_copy should have the value before increment

    ++it;
    ASSERT_EQ(it, batches.end());
}

TEST(BatcherTest, IteratorComparison) {
    std::vector<int> values = {1, 2, 3};
    size_t chunk_size = 1;
    auto batches1 = batcher(values, chunk_size);
    auto batches2 = batcher(values, chunk_size); // Different BatchView, same underlying data view

    auto it1_begin = batches1.begin();
    auto it1_end = batches1.end();
    auto it2_begin = batches2.begin();

    EXPECT_EQ(it1_begin, it2_begin); // Iterators from different BatchViews can be equal if they point to the same position

    auto it1_copy = it1_begin;
    EXPECT_EQ(it1_begin, it1_copy);
    ++it1_copy;
    EXPECT_NE(it1_begin, it1_copy);

    int count = 0;
    while(it1_begin != it1_end) {
        ++it1_begin;
        count++;
    }
    EXPECT_EQ(count, 3); // Should iterate 3 times
    EXPECT_EQ(it1_begin, it1_end);
}

TEST(BatcherTest, ConstIteratorFunctionality) {
    const std::vector<int> values = {1, 2, 3, 4, 5};
    size_t chunk_size = 2;
    auto batches = batcher(values, chunk_size); // uses const_iterator

    auto it = batches.begin(); // or batches.cbegin()
    ASSERT_NE(it, batches.end());
    EXPECT_EQ(*it, std::vector<int>({1, 2}));

    ++it;
    ASSERT_NE(it, batches.end());
    EXPECT_EQ(*it, std::vector<int>({3, 4}));

    it++; // Test post-increment
    ASSERT_NE(it, batches.end());
    EXPECT_EQ(*it, std::vector<int>({5}));

    ++it;
    ASSERT_EQ(it, batches.end()); // or batches.cend()
}

// Test with a container that might have non-contiguous iterators (like std::list)
// to ensure the iterator logic in BatchIterator is sound.
TEST(BatcherTest, ListIteratorFunctionality) {
    std::list<int> values = {1, 2, 3, 4, 5};
    size_t chunk_size = 2;
    auto batches = batcher(values, chunk_size);

    auto it = batches.begin();
    ASSERT_NE(it, batches.end());
    EXPECT_EQ(*it, std::vector<int>({1, 2}));

    ++it;
    ASSERT_NE(it, batches.end());
    EXPECT_EQ(*it, std::vector<int>({3, 4}));

    it++;
    ASSERT_NE(it, batches.end());
    EXPECT_EQ(*it, std::vector<int>({5}));

    ++it;
    ASSERT_EQ(it, batches.end());
}


// It is an error to create a batcher with chunk_size 0.
// The assertions in the code should catch this.
// For GTest, we can use ASSERT_DEATH if assertions are enabled and fatal.
// If they are not fatal, this test might not behave as expected across all build configurations.
// For now, we assume assertions are active and will cause termination.
// If not, this test will need adjustment or removal.
#if GTEST_HAS_DEATH_TEST
TEST(BatcherTest, ChunkSizeZeroDeath) {
    std::vector<int> values = {1, 2, 3};
    EXPECT_DEATH(batcher(values, 0), "Chunk size must be greater than 0");

    const std::vector<int> const_values = {1, 2, 3};
    EXPECT_DEATH(batcher(const_values, 0), "Chunk size must be greater than 0");
}
#endif

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
