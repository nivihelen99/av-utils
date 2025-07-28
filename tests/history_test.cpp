#include "gtest/gtest.h"
#include "history.h"
#include <string>
#include <vector>

TEST(HistoryTest, BasicOperations) {
    cpp_collections::History<int> history(10);

    ASSERT_EQ(history.versions(), 1);
    ASSERT_EQ(history.latest(), 10);
    ASSERT_EQ(history.current_version(), 0);

    history.latest() = 20;
    history.commit();

    ASSERT_EQ(history.versions(), 2);
    ASSERT_EQ(history.latest(), 20);
    ASSERT_EQ(history.current_version(), 1);
    ASSERT_EQ(history.get(0), 10);
    ASSERT_EQ(history.get(1), 20);

    history.latest() = 30;
    history.commit();

    ASSERT_EQ(history.versions(), 3);
    ASSERT_EQ(history.latest(), 30);
    ASSERT_EQ(history.current_version(), 2);
    ASSERT_EQ(history.get(0), 10);
    ASSERT_EQ(history.get(1), 20);
    ASSERT_EQ(history.get(2), 30);
}

TEST(HistoryTest, Revert) {
    cpp_collections::History<std::vector<std::string>> history({"a", "b"});
    history.commit(); // v1
    history.latest().push_back("c");
    history.commit(); // v2

    ASSERT_EQ(history.versions(), 3);
    ASSERT_EQ(history.latest(), std::vector<std::string>({"a", "b", "c"}));

    history.revert(1);
    ASSERT_EQ(history.versions(), 4);
    ASSERT_EQ(history.latest(), std::vector<std::string>({"a", "b"}));
    ASSERT_EQ(history.current_version(), 3);
    ASSERT_EQ(history.get(3), history.get(1));
}

TEST(HistoryTest, ClearAndReset) {
    cpp_collections::History<int> history(5);
    history.commit();
    history.latest() = 10;
    history.commit();

    ASSERT_EQ(history.versions(), 3);
    ASSERT_EQ(history.latest(), 10);

    history.clear();

    ASSERT_EQ(history.versions(), 1);
    ASSERT_EQ(history.latest(), 10);

    history.reset();
    ASSERT_EQ(history.versions(), 1);
    ASSERT_EQ(history.latest(), 0);
}

TEST(HistoryTest, OutOfRange) {
    cpp_collections::History<int> history(1);
    ASSERT_THROW(history.get(1), std::out_of_range);
    ASSERT_THROW(history.revert(1), std::out_of_range);
}

TEST(HistoryTest, DefaultConstructor) {
    cpp_collections::History<std::string> history;
    ASSERT_EQ(history.versions(), 1);
    ASSERT_EQ(history.latest(), "");
    history.latest() = "hello";
    history.commit();
    ASSERT_EQ(history.versions(), 2);
    ASSERT_EQ(history.get(0), "");
    ASSERT_EQ(history.get(1), "hello");
}
