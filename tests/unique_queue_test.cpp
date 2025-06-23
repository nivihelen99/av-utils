#include "gtest/gtest.h"
#include "unique_queue.h"
#include <string>
#include <vector>
#include <memory> // For std::unique_ptr
#include <algorithm> // For std::sort, std::transform
#include <functional> // For std::hash, std::equal_to

// Custom struct for testing
struct CustomData {
    int id;
    std::string name;

    CustomData(int i = 0, std::string n = "") : id(i), name(std::move(n)) {}

    // Equality based on id only for some tests
    bool operator==(const CustomData& other) const {
        return id == other.id;
    }
};

// Custom hash for CustomData based on id
struct CustomDataHash {
    std::size_t operator()(const CustomData& cd) const {
        return std::hash<int>{}(cd.id);
    }
};


// Test fixture for UniqueQueue
class UniqueQueueTest : public ::testing::Test {
protected:
    UniqueQueue<int> q_int_;
    UniqueQueue<std::string> q_str_;
    UniqueQueue<std::unique_ptr<int>> q_uptr_;
    UniqueQueue<CustomData, CustomDataHash> q_custom_; // Uses custom hash, default equality
};

TEST_F(UniqueQueueTest, InitialState) {
    EXPECT_TRUE(q_int_.empty());
    EXPECT_EQ(q_int_.size(), 0);
    EXPECT_FALSE(q_int_.contains(1));
}

TEST_F(UniqueQueueTest, PushBasicLValue) {
    int val1 = 1, val2 = 2;
    EXPECT_TRUE(q_int_.push(val1));
    EXPECT_EQ(q_int_.size(), 1);
    EXPECT_TRUE(q_int_.contains(val1));

    EXPECT_FALSE(q_int_.push(val1)); // Duplicate
    EXPECT_EQ(q_int_.size(), 1);

    EXPECT_TRUE(q_int_.push(val2));
    EXPECT_EQ(q_int_.size(), 2);
    EXPECT_TRUE(q_int_.contains(val2));
}

TEST_F(UniqueQueueTest, PushBasicRValue) {
    EXPECT_TRUE(q_int_.push(1));
    EXPECT_EQ(q_int_.size(), 1);
    EXPECT_TRUE(q_int_.contains(1));

    EXPECT_FALSE(q_int_.push(1)); // Duplicate
    EXPECT_EQ(q_int_.size(), 1);

    EXPECT_TRUE(q_int_.push(2));
    EXPECT_EQ(q_int_.size(), 2);
    EXPECT_TRUE(q_int_.contains(2));
}

TEST_F(UniqueQueueTest, PopAndFront) {
    q_str_.push("hello");
    q_str_.push("world");
    q_str_.push("hello"); // Duplicate

    ASSERT_EQ(q_str_.size(), 2);
    EXPECT_EQ(q_str_.front(), "hello");

    EXPECT_EQ(q_str_.pop(), "hello");
    ASSERT_EQ(q_str_.size(), 1);
    EXPECT_FALSE(q_str_.contains("hello"));
    EXPECT_TRUE(q_str_.contains("world"));
    EXPECT_EQ(q_str_.front(), "world");

    EXPECT_EQ(q_str_.pop(), "world");
    ASSERT_EQ(q_str_.size(), 0);
    EXPECT_FALSE(q_str_.contains("world"));
    EXPECT_TRUE(q_str_.empty());
}

TEST_F(UniqueQueueTest, PopEmpty) {
    EXPECT_THROW(q_int_.pop(), std::runtime_error);
    EXPECT_THROW(q_int_.front(), std::runtime_error);
}

TEST_F(UniqueQueueTest, TryPop) {
    EXPECT_FALSE(q_int_.try_pop().has_value());

    q_int_.push(10);
    auto val = q_int_.try_pop();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), 10);
    EXPECT_TRUE(q_int_.empty());
    EXPECT_FALSE(q_int_.try_pop().has_value());
}

TEST_F(UniqueQueueTest, Contains) {
    q_str_.push("test");
    EXPECT_TRUE(q_str_.contains("test"));
    EXPECT_FALSE(q_str_.contains("nonexistent"));
    q_str_.pop();
    EXPECT_FALSE(q_str_.contains("test"));
}

TEST_F(UniqueQueueTest, Clear) {
    q_int_.push(1);
    q_int_.push(2);
    ASSERT_FALSE(q_int_.empty());
    q_int_.clear();
    EXPECT_TRUE(q_int_.empty());
    EXPECT_EQ(q_int_.size(), 0);
    EXPECT_FALSE(q_int_.contains(1));
    EXPECT_FALSE(q_int_.contains(2));
    // Can push again after clear
    EXPECT_TRUE(q_int_.push(1));
    EXPECT_EQ(q_int_.size(), 1);
}

TEST_F(UniqueQueueTest, Remove) {
    q_int_.push(1);
    q_int_.push(2);
    q_int_.push(3);
    q_int_.push(4);
    q_int_.push(2); // duplicate, not added

    ASSERT_EQ(q_int_.size(), 4);
    EXPECT_TRUE(q_int_.remove(2)); // Remove 2
    EXPECT_EQ(q_int_.size(), 3);
    EXPECT_FALSE(q_int_.contains(2));

    // Check order
    EXPECT_EQ(q_int_.pop(), 1);
    EXPECT_EQ(q_int_.pop(), 3);
    EXPECT_EQ(q_int_.pop(), 4);
    EXPECT_TRUE(q_int_.empty());

    // Remove from empty or non-existent
    EXPECT_FALSE(q_int_.remove(100));
    q_int_.push(5);
    EXPECT_FALSE(q_int_.remove(100));
    EXPECT_TRUE(q_int_.remove(5));
}

TEST_F(UniqueQueueTest, RemoveCorrectlyHandlesInternalQueue) {
    q_int_.push(1);
    q_int_.push(2);
    q_int_.push(3);
    q_int_.push(2); // This push is ignored
    q_int_.push(4);
    q_int_.push(5);
    // Queue: 1, 2, 3, 4, 5. Seen: {1,2,3,4,5}

    EXPECT_TRUE(q_int_.remove(3)); // Remove 3
    // Queue should be: 1, 2, 4, 5. Seen: {1,2,4,5}
    EXPECT_EQ(q_int_.size(), 4);
    EXPECT_FALSE(q_int_.contains(3));

    EXPECT_EQ(q_int_.pop(), 1);
    EXPECT_EQ(q_int_.pop(), 2);
    EXPECT_EQ(q_int_.pop(), 4);
    EXPECT_EQ(q_int_.pop(), 5);
    EXPECT_TRUE(q_int_.empty());
}


TEST_F(UniqueQueueTest, PushUniquePtrRValue) {
    // This test is crucial due to the tricky std::move in unique_queue's push(T&&)
    q_uptr_.push(std::make_unique<int>(10));
    q_uptr_.push(std::make_unique<int>(20));

    // The following unique_ptr has the same underlying value (10) as the first one,
    // but it's a distinct object. For unique_ptr, equality is pointer equality.
    // However, the set `seen_` will store unique_ptr by value.
    // If T is unique_ptr<int>, seen_ stores unique_ptr<int>.
    // The `count` and `insert` for unique_ptr will work on pointer address, not pointed-to value.
    // This means pushing a new unique_ptr pointing to the same value as an existing one
    // *should* be considered a new element IF unique_ptr's hash/equality are default.
    // However, if we want uniqueness based on *pointed-to value*, we need custom hash/equal.
    // For this test, using default hash/equal for unique_ptr.

    auto uptr_dup_val = std::make_unique<int>(10);
    // This is a new unique_ptr, so it should be added.
    // The provided UniqueQueue code for push(T&&) has a comment about unique_ptr behavior.
    // seen_.insert(std::move(value)); queue_.push(std::move(value));
    // If `value` is `uptr_dup_val`, after `seen_.insert`, `uptr_dup_val` is moved-from.
    // Then `queue_.push(std::move(uptr_dup_val))` pushes a nullptr. This is bad.
    // The current unique_queue.h has this potential issue.
    // The fix for the compilation of `seen_.insert(value)` to `seen_.insert(std::move(value))`
    // might expose this runtime issue for unique_ptr.

    // Let's assume default unique_ptr hashing/equality (pointer based).
    // Then a unique_ptr pointing to a new int(10) is different from the first unique_ptr.
    EXPECT_TRUE(q_uptr_.push(std::make_unique<int>(10))) << "Pushing unique_ptr with same pointed-to value but different object should succeed with default hash/equal";
    EXPECT_EQ(q_uptr_.size(), 3);

    auto p1 = q_uptr_.pop();
    ASSERT_NE(p1, nullptr); EXPECT_EQ(*p1, 10);

    auto p2 = q_uptr_.pop();
    ASSERT_NE(p2, nullptr); EXPECT_EQ(*p2, 20);

    auto p3 = q_uptr_.pop(); // This would be the second unique_ptr pointing to 10
    ASSERT_NE(p3, nullptr); EXPECT_EQ(*p3, 10);
}

TEST_F(UniqueQueueTest, CustomTypeAndHash) {
    q_custom_.push(CustomData(1, "Alice"));
    q_custom_.push(CustomData(2, "Bob"));
    EXPECT_FALSE(q_custom_.push(CustomData(1, "Alicia"))); // Duplicate ID

    ASSERT_EQ(q_custom_.size(), 2);
    EXPECT_TRUE(q_custom_.contains(CustomData(1))); // Check by ID
    EXPECT_TRUE(q_custom_.contains(CustomData(2, "Robert"))); // Check by ID
    EXPECT_FALSE(q_custom_.contains(CustomData(3)));

    CustomData cd1 = q_custom_.pop();
    EXPECT_EQ(cd1.id, 1);
    EXPECT_EQ(cd1.name, "Alice"); // Original name preserved

    CustomData cd2 = q_custom_.pop();
    EXPECT_EQ(cd2.id, 2);
}

TEST_F(UniqueQueueTest, CopySemantics) {
    UniqueQueue<int> q1;
    q1.push(10);
    q1.push(20);

    UniqueQueue<int> q2 = q1; // Copy constructor
    EXPECT_EQ(q2.size(), 2);
    EXPECT_TRUE(q2.contains(10));
    EXPECT_TRUE(q2.contains(20));
    q1.pop(); // Modify q1
    EXPECT_EQ(q1.size(), 1);
    EXPECT_EQ(q2.size(), 2); // q2 should be unaffected

    UniqueQueue<int> q3;
    q3.push(30);
    q3 = q2; // Copy assignment
    EXPECT_EQ(q3.size(), 2);
    q2.pop();
    EXPECT_EQ(q2.size(), 1);
    EXPECT_EQ(q3.size(), 2); // q3 should be unaffected
    EXPECT_EQ(q3.pop(), 10);
    EXPECT_EQ(q3.pop(), 20);
}

TEST_F(UniqueQueueTest, MoveSemanticsQueue) {
    UniqueQueue<std::string> q1;
    q1.push("one");
    q1.push("two");

    UniqueQueue<std::string> q2 = std::move(q1); // Move constructor
    EXPECT_EQ(q2.size(), 2);
    EXPECT_TRUE(q1.empty()); // q1 should be empty
    EXPECT_EQ(q2.pop(), "one");

    UniqueQueue<std::string> q3;
    q3.push("three");
    q3 = std::move(q2); // Move assignment
    EXPECT_EQ(q3.size(), 1);
    EXPECT_TRUE(q2.empty()); // q2 should be empty
    EXPECT_EQ(q3.pop(), "two");
}

TEST_F(UniqueQueueTest, Swap) {
    q_int_.push(1);
    q_int_.push(2);

    UniqueQueue<int> q_other;
    q_other.push(3);
    q_other.push(4);
    q_other.push(5);

    q_int_.swap(q_other);

    EXPECT_EQ(q_int_.size(), 3);
    EXPECT_TRUE(q_int_.contains(3));
    EXPECT_FALSE(q_int_.contains(1));

    EXPECT_EQ(q_other.size(), 2);
    EXPECT_TRUE(q_other.contains(1));
    EXPECT_FALSE(q_other.contains(3));

    EXPECT_EQ(q_int_.pop(), 3);
    EXPECT_EQ(q_other.pop(), 1);
}

TEST_F(UniqueQueueTest, IteratorBasic) {
    q_int_.push(1);
    q_int_.push(2);
    q_int_.push(1); // duplicate
    q_int_.push(3);

    std::vector<int> expected = {1, 2, 3};
    std::vector<int> actual;
    for (const auto& item : q_int_) {
        actual.push_back(item);
    }
    EXPECT_EQ(actual, expected);
}

TEST_F(UniqueQueueTest, IteratorEmptyQueue) {
    std::vector<int> actual;
    for (const auto& item : q_int_) {
        actual.push_back(item);
    }
    EXPECT_TRUE(actual.empty());
}

TEST_F(UniqueQueueTest, MaxSize) {
    // This is a bit trivial, just checks it doesn't crash and returns something.
    EXPECT_GT(q_int_.max_size(), 0);
}

// Test for std::equal_to and std::hash with non-primitive types
struct ComplexKey {
    int id;
    std::string name;
    bool operator==(const ComplexKey& other) const { return id == other.id && name == other.name; }
};
namespace std {
    template <> struct hash<ComplexKey> {
        size_t operator()(const ComplexKey& k) const {
            return hash<int>()(k.id) ^ (hash<string>()(k.name) << 1);
        }
    };
}

TEST_F(UniqueQueueTest, ComplexKeyWithStdHashAndEqualTo) {
    UniqueQueue<ComplexKey> q_complex;
    EXPECT_TRUE(q_complex.push({1, "A"}));
    EXPECT_TRUE(q_complex.push({2, "B"}));
    EXPECT_FALSE(q_complex.push({1, "A"})); // Duplicate
    EXPECT_TRUE(q_complex.push({1, "C"}));  // Different name, different key

    EXPECT_EQ(q_complex.size(), 3);
    EXPECT_TRUE(q_complex.contains({1, "A"}));
    EXPECT_TRUE(q_complex.contains({1, "C"}));
    EXPECT_FALSE(q_complex.contains({3, "D"}));

    ComplexKey k = q_complex.pop();
    EXPECT_EQ(k.id, 1); EXPECT_EQ(k.name, "A");
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
