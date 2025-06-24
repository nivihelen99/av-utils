#include "gtest/gtest.h"
#include "peekable.h" // Assuming this path will be configured in CMake include_directories
#include <vector>
#include <string>
#include <deque>
#include <list> // For testing with std::list (bidirectional iterator)
#include <sstream> // For testing with stream iterators (input iterator)

// Test fixture for Peekable tests
class PeekableTest : public ::testing::Test {
protected:
    // You can put common setup/teardown here if needed
};

// Test case for basic operations with std::vector<int>
TEST_F(PeekableTest, BasicOperationsVectorInt) {
    std::vector<int> data = {1, 2, 3};
    auto p = utils::make_peekable(data);

    ASSERT_TRUE(p.has_next());
    ASSERT_EQ(p.peek(), 1);
    ASSERT_TRUE(p.has_next()); // Peeking should not consume
    ASSERT_EQ(p.peek(), 1);

    ASSERT_EQ(p.next(), 1);
    ASSERT_TRUE(p.has_next());
    ASSERT_EQ(p.peek(), 2);

    p.consume(); // Consume 2
    ASSERT_TRUE(p.has_next());
    ASSERT_EQ(p.peek(), 3);

    ASSERT_EQ(p.next(), 3);
    ASSERT_FALSE(p.has_next());
    ASSERT_EQ(p.peek(), std::nullopt); // Or check has_value() == false
    ASSERT_EQ(p.next(), std::nullopt);
}

// Test case for empty container
TEST_F(PeekableTest, EmptyContainer) {
    std::vector<int> data = {};
    auto p = utils::make_peekable(data);

    ASSERT_FALSE(p.has_next());
    ASSERT_EQ(p.peek(), std::nullopt);
    ASSERT_EQ(p.next(), std::nullopt);
}

// Test with std::deque<std::string>
TEST_F(PeekableTest, DequeString) {
    std::deque<std::string> data = {"hello", "world", "test"};
    auto p = utils::make_peekable(data);

    ASSERT_TRUE(p.has_next());
    ASSERT_EQ(p.peek(), "hello");
    ASSERT_EQ(p.next(), "hello");

    ASSERT_TRUE(p.has_next());
    ASSERT_EQ(p.peek(), "world");
    ASSERT_EQ(p.next(), "world");

    ASSERT_TRUE(p.has_next());
    ASSERT_EQ(p.peek(), "test");
    ASSERT_EQ(p.next(), "test");

    ASSERT_FALSE(p.has_next());
}

// Test iterator-style usage
TEST_F(PeekableTest, IteratorStyle) {
    std::vector<int> data = {10, 20};
    auto p = utils::make_peekable(data);
    auto p_end = utils::Peekable<std::vector<int>::iterator>(data.end(), data.end());


    ASSERT_NE(p, p_end); // Assuming Peekable defines operator!=
    ASSERT_TRUE(p.has_next());
    ASSERT_EQ(*p, 10); // Dereference

    ++p; // Increment
    ASSERT_TRUE(p.has_next());
    ASSERT_EQ(*p, 20);

    p++; // Post-increment
    ASSERT_FALSE(p.has_next()); // Now it should be at the end
    ASSERT_EQ(p, p_end); // Should be equal to an "end" peekable
}

// Test with input iterators (e.g., std::istream_iterator)
TEST_F(PeekableTest, InputIteratorStream) {
    std::istringstream iss("one two three");
    // Use uniform initialization to avoid vexing parse
    utils::Peekable<std::istream_iterator<std::string>> p{
        std::istream_iterator<std::string>(iss),
        std::istream_iterator<std::string>()
    };

    ASSERT_TRUE(p.has_next());
    ASSERT_EQ(p.peek(), "one");
    ASSERT_EQ(p.next(), "one");

    ASSERT_TRUE(p.has_next());
    ASSERT_EQ(p.peek(), "two");
    ASSERT_EQ(p.next(), "two");

    ASSERT_TRUE(p.has_next());
    ASSERT_EQ(p.peek(), "three");
    ASSERT_EQ(p.next(), "three");

    ASSERT_FALSE(p.has_next());
}

// Test make_peekable with const container
TEST_F(PeekableTest, ConstContainer) {
    const std::vector<int> data = {5, 10, 15};
    auto p = utils::make_peekable(data); // Should use const_iterator

    ASSERT_TRUE(p.has_next());
    ASSERT_EQ(p.peek(), 5);
    ASSERT_EQ(p.next(), 5);
    ASSERT_EQ(p.next(), 10);
    ASSERT_EQ(p.next(), 15);
    ASSERT_FALSE(p.has_next());
}

// Test peek_n functionality
TEST_F(PeekableTest, PeekNFunctionality) {
    std::vector<int> data = {1, 2, 3, 4, 5};
    auto p = utils::make_peekable(data);

    ASSERT_TRUE(decltype(p)::has_peek_n()); // Vector iterators are random access

    EXPECT_EQ(p.peek(), 1);
    EXPECT_EQ(p.peek_n(0), 1); // Peek at current buffered
    EXPECT_EQ(p.peek_n(1), 2); // Peek one ahead
    EXPECT_EQ(p.peek_n(2), 3); // Peek two ahead
    EXPECT_EQ(p.peek_n(4), 5); // Peek to the last element
    EXPECT_EQ(p.peek_n(5), std::nullopt); // Peek beyond the end
    EXPECT_EQ(p.peek_n(100), std::nullopt); // Peek way beyond

    // Consume one element
    EXPECT_EQ(p.next(), 1);
    EXPECT_EQ(p.peek(), 2);
    EXPECT_EQ(p.peek_n(0), 2);
    EXPECT_EQ(p.peek_n(1), 3);
    EXPECT_EQ(p.peek_n(3), 5);
    EXPECT_EQ(p.peek_n(4), std::nullopt);

    // Consume all
    p.next(); p.next(); p.next(); p.next(); // 2, 3, 4, 5
    ASSERT_FALSE(p.has_next());
    EXPECT_EQ(p.peek_n(0), std::nullopt);
    EXPECT_EQ(p.peek_n(1), std::nullopt);
}

// Test peek_n with an iterator type that might not support it fully (e.g. input iterator)
// For input iterators, peek_n is SFINAE'd out.
// For forward iterators (like std::list), it should work.
TEST_F(PeekableTest, PeekNWithForwardIterator) {
    std::list<int> data = {10, 20, 30}; // std::list has bidirectional iterators
    auto p = utils::make_peekable(data);

    ASSERT_TRUE(decltype(p)::has_peek_n());

    EXPECT_EQ(p.peek(), 10);
    EXPECT_EQ(p.peek_n(0), 10);
    EXPECT_EQ(p.peek_n(1), 20);
    EXPECT_EQ(p.peek_n(2), 30);
    EXPECT_EQ(p.peek_n(3), std::nullopt);

    EXPECT_EQ(p.next(), 10);
    EXPECT_EQ(p.peek_n(0), 20);
    EXPECT_EQ(p.peek_n(1), 30);
}

TEST_F(PeekableTest, PeekNWithInputIteratorDisabled) {
    std::istringstream iss("one two three");
    // Use uniform initialization to avoid vexing parse
    utils::Peekable<std::istream_iterator<std::string>> p{
        std::istream_iterator<std::string>(iss),
        std::istream_iterator<std::string>()
    };
    // peek_n should not be available for input iterators due to SFINAE
    ASSERT_FALSE(decltype(p)::has_peek_n());

    // Attempting to call it directly would be a compile error if SFINAE works.
    // We can't directly test the compile error here, but has_peek_n() confirms intent.
    // If we were to try: (this would fail to compile as desired)
    // if constexpr (decltype(p)::has_peek_n()) {
    //    p.peek_n(1);
    // }
    // So we just check has_next and next
    ASSERT_TRUE(p.has_next());
    ASSERT_EQ(p.next(), "one");
}


// Test PeekableRange for range-based for loops
TEST_F(PeekableTest, RangeBasedForLoop) {
    std::vector<int> data = {100, 200, 300};
    std::vector<int> result;

    // The PeekableRange itself is not directly iterated.
    // Instead, its begin() returns a Peekable which is then used.
    // The range-based for loop works on the Peekable's iterator interface.
    // for (auto val : utils::peekable_range(data)) { // val would be Peekable<Iterator>
    // This usage is tricky because the value type of PeekableRange::begin() is Peekable itself.
    // A more direct way to test range-based for is to use the Peekable directly if it's an iterator.

    auto p_begin = utils::make_peekable(data);
    // Create an "end" peekable. Its 'current_' should be data.end().
    auto p_end_iter = data.end();
    utils::Peekable<std::vector<int>::iterator> p_end(p_end_iter, p_end_iter);

    int loop_count = 0;
    for (auto it = p_begin; it != p_end; ++it) {
        result.push_back(*it); // *it calls Peekable::operator* which calls peek()
        loop_count++;
        if (loop_count > data.size()) { // Safety break
            FAIL() << "Loop seems to be infinite";
            break;
        }
    }

    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], 100);
    EXPECT_EQ(result[1], 200);
    EXPECT_EQ(result[2], 300);
}

// Test multiple peeks and then a next
TEST_F(PeekableTest, MultiplePeeksThenNext) {
    std::vector<int> data = {1, 2, 3};
    auto p = utils::make_peekable(data);

    ASSERT_EQ(p.peek(), 1);
    ASSERT_EQ(p.peek(), 1); // Second peek, same result
    ASSERT_EQ(p.peek(), 1); // Third peek

    ASSERT_EQ(p.next(), 1); // Consume

    ASSERT_EQ(p.peek(), 2);
    ASSERT_EQ(p.next(), 2);

    ASSERT_EQ(p.peek(), 3);
    ASSERT_EQ(p.next(), 3);

    ASSERT_FALSE(p.has_next());
}

// Test that buffer is correctly managed
TEST_F(PeekableTest, BufferManagement) {
    std::vector<int> data = {10, 20};
    auto p = utils::make_peekable(data);

    // Initially, buffer is not filled.
    // First call to has_next() or peek() or next() fills it.
    ASSERT_EQ(p.peek(), 10); // Fills buffer with 10

    // current_ iterator is still at begin, but buffer is full.
    // If we could inspect p.buffer_filled_ it would be true.
    // And p.buffer_ would contain 10.

    ASSERT_EQ(p.next(), 10); // Consumes from buffer, then advances current_ to point to 20.
                             // Buffer becomes empty.

    // Now current_ points to 20. Buffer is empty.
    ASSERT_EQ(p.peek(), 20); // Fills buffer with 20.
    ASSERT_EQ(p.next(), 20); // Consumes from buffer. Advances current_ to end.

    ASSERT_FALSE(p.has_next());
}

// Test with a container that has only one element
TEST_F(PeekableTest, SingleElementContainer) {
    std::vector<std::string> data = {"lonely"};
    auto p = utils::make_peekable(data);

    ASSERT_TRUE(p.has_next());
    ASSERT_EQ(p.peek(), "lonely");
    ASSERT_EQ(p.next(), "lonely");
    ASSERT_FALSE(p.has_next());
    ASSERT_EQ(p.peek(), std::nullopt);
    ASSERT_EQ(p.next(), std::nullopt);
}

// Test consume method
TEST_F(PeekableTest, ConsumeMethod) {
    std::vector<int> data = {1, 2, 3, 4};
    auto p = utils::make_peekable(data);

    ASSERT_TRUE(p.has_next());
    ASSERT_EQ(p.peek(), 1);

    p.consume(); // Consumes 1
    ASSERT_TRUE(p.has_next());
    ASSERT_EQ(p.peek(), 2);

    p.consume(); // Consumes 2
    ASSERT_TRUE(p.has_next());
    ASSERT_EQ(p.peek(), 3);

    ASSERT_EQ(p.next(), 3); // Consumes 3 using next()

    ASSERT_TRUE(p.has_next());
    ASSERT_EQ(p.peek(), 4);
    p.consume(); // Consumes 4

    ASSERT_FALSE(p.has_next());
}

// Test base() method
TEST_F(PeekableTest, BaseMethod) {
    std::vector<int> data = {1, 2, 3};
    auto it_begin = data.begin();
    auto it_end = data.end();

    utils::Peekable<std::vector<int>::iterator> p(it_begin, it_end);

    // Before any operation, base() should be the initial iterator.
    // However, Peekable fills its buffer on construction if possible, or on first access.
    // The `current_` member is advanced when `next()` is called and the buffer is emptied.
    // Let's trace:
    // 1. Peekable(it_begin, it_end) -> current_ = it_begin, buffer_empty
    // 2. p.peek() -> fill_buffer() -> buffer_ = *current_ (1), buffer_filled_ = true. current_ is still it_begin.
    //    base() returns current_, which is it_begin.
    EXPECT_EQ(p.base(), it_begin); // After peek, current_ is not advanced yet by fill_buffer
                                   // but buffer is filled. This is subtle.
                                   // The `current_` in Peekable points to the *next* item to buffer.
                                   // So if buffer has '1', current_ points to '2'.

    // Let's re-verify `base()` behavior.
    // `fill_buffer()` reads `*current_` then `buffer_filled_ = true`. `current_` itself isn't advanced by `fill_buffer`.
    // `next()`: calls `fill_buffer()`. Then `++current_`.
    // So `base()` returns the iterator to the element *after* the one that was last returned by `next()`,
    // or the one that would be returned by `next()` if `peek()` was called.

    // Scenario 1: Fresh Peekable
    utils::Peekable p1(data.begin(), data.end());
    // p1.current_ is data.begin(). Buffer is empty.
    // base() returns data.begin().
    EXPECT_EQ(p1.base(), data.begin());


    // Scenario 2: After peek()
    utils::Peekable p2(data.begin(), data.end());
    p2.peek(); // buffer gets 1. p2.current_ is still data.begin().
    EXPECT_EQ(p2.base(), data.begin());


    // Scenario 3: After next()
    utils::Peekable p3(data.begin(), data.end());
    p3.next(); // returns 1. p3.current_ is now data.begin()+1.
    EXPECT_EQ(p3.base(), data.begin() + 1);

    // Scenario 4: After multiple next()
    utils::Peekable p4(data.begin(), data.end());
    p4.next(); // 1
    p4.next(); // 2
    EXPECT_EQ(p4.base(), data.begin() + 2);
    p4.next(); // 3
    EXPECT_EQ(p4.base(), data.begin() + 3); // data.end()
    EXPECT_EQ(p4.base(), data.end());

    // Scenario 5: After consuming all
    utils::Peekable p5(data.begin(), data.end());
    while(p5.has_next()) p5.next();
    EXPECT_EQ(p5.base(), data.end());
}
// TODO: Add more tests if specific edge cases or behaviors are identified.
// For example, interaction between peek_n and next/consume.

TEST_F(PeekableTest, PeekNInteractionWithNext) {
    std::vector<int> data = {1, 2, 3, 4, 5, 6};
    auto p = utils::make_peekable(data);

    // Initial state: p.peek() is 1. p.current_ points to 1 (or rather, begin())
    EXPECT_EQ(p.peek(), 1);         // Buffer filled with 1. current_ still at begin().
    EXPECT_EQ(p.peek_n(0), 1);      // Uses buffer.
    EXPECT_EQ(p.peek_n(1), 2);      // Looks ahead from current_ (which is begin()) + 1
    EXPECT_EQ(p.peek_n(2), 3);

    // Consume 1
    EXPECT_EQ(p.next(), 1);         // Buffer emptied. current_ advanced to point to 2.
                                    // peek() will now buffer 2.

    EXPECT_EQ(p.peek(), 2);         // Buffer filled with 2. current_ still points to 2.
    EXPECT_EQ(p.peek_n(0), 2);      // Uses buffer.
    EXPECT_EQ(p.peek_n(1), 3);      // Looks ahead from current_ (points to 2) + 1 = 3
    EXPECT_EQ(p.peek_n(2), 4);

    // Peek far ahead, then consume
    EXPECT_EQ(p.peek_n(3), 5);      // current_ still points to 2.
    EXPECT_EQ(p.next(), 2);         // Buffer (2) emptied. current_ advanced to point to 3.

    EXPECT_EQ(p.peek(), 3);
    EXPECT_EQ(p.peek_n(0), 3);
    EXPECT_EQ(p.peek_n(1), 4);
    EXPECT_EQ(p.peek_n(2), 5);
    EXPECT_EQ(p.peek_n(3), 6);
    EXPECT_EQ(p.peek_n(4), std::nullopt);

    // Consume until near end
    EXPECT_EQ(p.next(), 3);
    EXPECT_EQ(p.next(), 4);
    EXPECT_EQ(p.next(), 5);
    // Now p.peek() is 6. current_ points to 6.

    EXPECT_EQ(p.peek(), 6);
    EXPECT_EQ(p.peek_n(0), 6);
    EXPECT_EQ(p.peek_n(1), std::nullopt); // No element after 6

    EXPECT_EQ(p.next(), 6);
    ASSERT_FALSE(p.has_next());
    EXPECT_EQ(p.peek_n(0), std::nullopt);
}
