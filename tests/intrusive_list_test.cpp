#include "intrusive_list.h"
#include "gtest/gtest.h"

namespace {

struct TestObject {
    int value;
    cpp_utils::intrusive_list_hook hook;

    TestObject(int v) : value(v) {}
};

using TestList = cpp_utils::intrusive_list<TestObject, &TestObject::hook>;

TEST(IntrusiveListTest, InitialState) {
    TestList list;
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(0, list.size());
}

TEST(IntrusiveListTest, PushBack) {
    TestList list;
    TestObject obj1(10);
    TestObject obj2(20);

    list.push_back(obj1);
    EXPECT_FALSE(list.empty());
    EXPECT_EQ(1, list.size());
    EXPECT_EQ(10, list.front().value);
    EXPECT_EQ(10, list.back().value);

    list.push_back(obj2);
    EXPECT_EQ(2, list.size());
    EXPECT_EQ(10, list.front().value);
    EXPECT_EQ(20, list.back().value);
}

TEST(IntrusiveListTest, PushFront) {
    TestList list;
    TestObject obj1(10);
    TestObject obj2(20);

    list.push_front(obj1);
    EXPECT_FALSE(list.empty());
    EXPECT_EQ(1, list.size());
    EXPECT_EQ(10, list.front().value);
    EXPECT_EQ(10, list.back().value);

    list.push_front(obj2);
    EXPECT_EQ(2, list.size());
    EXPECT_EQ(20, list.front().value);
    EXPECT_EQ(10, list.back().value);
}

TEST(IntrusiveListTest, PopFront) {
    TestList list;
    TestObject obj1(10);
    TestObject obj2(20);
    list.push_back(obj1);
    list.push_back(obj2);

    list.pop_front();
    EXPECT_EQ(1, list.size());
    EXPECT_EQ(20, list.front().value);

    list.pop_front();
    EXPECT_TRUE(list.empty());
}

TEST(IntrusiveListTest, PopBack) {
    TestList list;
    TestObject obj1(10);
    TestObject obj2(20);
    list.push_back(obj1);
    list.push_back(obj2);

    list.pop_back();
    EXPECT_EQ(1, list.size());
    EXPECT_EQ(10, list.front().value);

    list.pop_back();
    EXPECT_TRUE(list.empty());
}

TEST(IntrusiveListTest, Erase) {
    TestList list;
    TestObject obj1(10);
    TestObject obj2(20);
    TestObject obj3(30);
    list.push_back(obj1);
    list.push_back(obj2);
    list.push_back(obj3);

    auto it = ++list.begin();
    list.erase(it);

    EXPECT_EQ(2, list.size());
    EXPECT_EQ(10, list.front().value);
    EXPECT_EQ(30, list.back().value);
}

TEST(IntrusiveListTest, Iterator) {
    TestList list;
    TestObject obj1(10);
    TestObject obj2(20);
    list.push_back(obj1);
    list.push_back(obj2);

    auto it = list.begin();
    EXPECT_EQ(10, it->value);
    ++it;
    EXPECT_EQ(20, it->value);
    ++it;
    EXPECT_EQ(list.end(), it);
}

TEST(IntrusiveListTest, ConstIterator) {
    TestList list;
    TestObject obj1(10);
    TestObject obj2(20);
    list.push_back(obj1);
    list.push_back(obj2);

    const TestList& const_list = list;
    auto it = const_list.cbegin();
    EXPECT_EQ(10, it->value);
    ++it;
    EXPECT_EQ(20, it->value);
    ++it;
    EXPECT_EQ(const_list.cend(), it);
}

TEST(IntrusiveListTest, Clear) {
    TestList list;
    TestObject obj1(10);
    TestObject obj2(20);
    list.push_back(obj1);
    list.push_back(obj2);

    list.clear();
    EXPECT_TRUE(list.empty());
    EXPECT_EQ(0, list.size());
    EXPECT_FALSE(obj1.hook.is_linked());
    EXPECT_FALSE(obj2.hook.is_linked());
}

} // namespace
