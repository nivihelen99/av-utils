#include "gtest/gtest.h"
#include "expiring_containers.h"
#include <thread>
#include <chrono>
#include <string>
#include <vector>

using namespace expiring;
using namespace std::chrono_literals;

// Helper function to sleep for a duration
void sleep_for_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// Tests for TimeStampedQueue
class TimeStampedQueueTest : public ::testing::Test {
protected:
    TimeStampedQueue<std::string> queue_{100ms}; // Default TTL of 100ms
};

TEST_F(TimeStampedQueueTest, InitialState) {
    ASSERT_TRUE(queue_.empty());
    ASSERT_EQ(queue_.size(), 0);
    ASSERT_EQ(queue_.get_ttl(), 100ms);
}

TEST_F(TimeStampedQueueTest, PushAndFront) {
    queue_.push("one");
    ASSERT_FALSE(queue_.empty());
    ASSERT_EQ(queue_.size(), 1);
    ASSERT_EQ(queue_.front(), "one");

    queue_.push("two");
    ASSERT_EQ(queue_.size(), 2);
    ASSERT_EQ(queue_.front(), "one"); // Still "one" because it's a FIFO queue
}

TEST_F(TimeStampedQueueTest, Pop) {
    queue_.push("one");
    queue_.push("two");
    ASSERT_EQ(queue_.pop(), "one");
    ASSERT_EQ(queue_.size(), 1);
    ASSERT_EQ(queue_.front(), "two");
    ASSERT_EQ(queue_.pop(), "two");
    ASSERT_TRUE(queue_.empty());
}

TEST_F(TimeStampedQueueTest, PopEmpty) {
    ASSERT_THROW(queue_.pop(), std::runtime_error);
}

TEST_F(TimeStampedQueueTest, FrontEmpty) {
    ASSERT_THROW(queue_.front(), std::runtime_error);
}

TEST_F(TimeStampedQueueTest, Expiration) {
    queue_.push("one");
    sleep_for_ms(50);
    queue_.push("two");
    ASSERT_EQ(queue_.size(), 2);

    sleep_for_ms(80); // "one" should expire (total 50+80 = 130ms > 100ms)
                      // "two" should still be there (total 80ms < 100ms)
    ASSERT_EQ(queue_.size(), 1);
    ASSERT_EQ(queue_.front(), "two");

    sleep_for_ms(50); // "two" should expire (total 80+50 = 130ms > 100ms)
    ASSERT_TRUE(queue_.empty());
    ASSERT_EQ(queue_.size(), 0);
}

TEST_F(TimeStampedQueueTest, Clear) {
    queue_.push("one");
    queue_.push("two");
    ASSERT_EQ(queue_.size(), 2);
    queue_.clear();
    ASSERT_TRUE(queue_.empty());
    ASSERT_EQ(queue_.size(), 0);
}

TEST_F(TimeStampedQueueTest, SetAndGetTtl) {
    queue_.set_ttl(200ms);
    ASSERT_EQ(queue_.get_ttl(), 200ms);

    queue_.push("test_ttl");
    sleep_for_ms(150);
    ASSERT_FALSE(queue_.empty()); // Should not expire yet

    sleep_for_ms(100); // Total 250ms, should expire now
    ASSERT_TRUE(queue_.empty());
}

TEST_F(TimeStampedQueueTest, PushRValue) {
    std::string s = "rvalue_test";
    queue_.push(std::move(s));
    ASSERT_FALSE(queue_.empty());
    ASSERT_EQ(queue_.front(), "rvalue_test");
    ASSERT_TRUE(s.empty()); // Check if moved
}

TEST_F(TimeStampedQueueTest, ExpireMethodDirectCall) {
    queue_.push("a");
    queue_.push("b");
    sleep_for_ms(120); // Both should be expired
    queue_.expire();   // Manually call expire
    ASSERT_TRUE(queue_.empty());
}

TEST_F(TimeStampedQueueTest, SizeAfterPartialExpiration) {
    queue_.set_ttl(50ms);
    queue_.push("1");
    sleep_for_ms(30);
    queue_.push("2");
    sleep_for_ms(30); // "1" expires (30+30=60 > 50), "2" does not (30 < 50)
    ASSERT_EQ(queue_.size(), 1);
    ASSERT_EQ(queue_.front(), "2");
}

// Tests for ExpiringDict
class ExpiringDictTest : public ::testing::Test {
protected:
    ExpiringDict<std::string, int> dict_{100ms, false}; // Default TTL 100ms, no access_renews
};

TEST_F(ExpiringDictTest, InitialState) {
    ASSERT_TRUE(dict_.empty());
    ASSERT_EQ(dict_.size(), 0);
    ASSERT_EQ(dict_.get_ttl(), 100ms);
    ASSERT_FALSE(dict_.get_access_renews());
}

TEST_F(ExpiringDictTest, InsertAndFind) {
    dict_.insert("one", 1);
    ASSERT_FALSE(dict_.empty());
    ASSERT_EQ(dict_.size(), 1);

    int* val = dict_.find("one");
    ASSERT_NE(val, nullptr);
    ASSERT_EQ(*val, 1);

    ASSERT_EQ(dict_.find("nonexistent"), nullptr);
}

TEST_F(ExpiringDictTest, InsertRValue) {
    std::string key = "key_rval";
    int val_rval = 123;
    dict_.insert(key, std::move(val_rval));

    int* found_val = dict_.find("key_rval");
    ASSERT_NE(found_val, nullptr);
    ASSERT_EQ(*found_val, 123);
}


TEST_F(ExpiringDictTest, Contains) {
    dict_.insert("one", 1);
    ASSERT_TRUE(dict_.contains("one"));
    ASSERT_FALSE(dict_.contains("nonexistent"));
}

TEST_F(ExpiringDictTest, Erase) {
    dict_.insert("one", 1);
    ASSERT_TRUE(dict_.contains("one"));
    ASSERT_TRUE(dict_.erase("one"));
    ASSERT_FALSE(dict_.contains("one"));
    ASSERT_FALSE(dict_.erase("nonexistent"));
}

TEST_F(ExpiringDictTest, Expiration) {
    dict_.insert("one", 1);
    sleep_for_ms(50);
    dict_.insert("two", 2);
    ASSERT_EQ(dict_.size(), 2);

    sleep_for_ms(80); // "one" should expire (50+80=130 > 100)
                      // "two" should not (80 < 100)
    ASSERT_EQ(dict_.size(), 1);
    ASSERT_TRUE(dict_.contains("two"));
    ASSERT_FALSE(dict_.contains("one"));
    ASSERT_EQ(dict_.find("one"), nullptr);

    sleep_for_ms(50); // "two" should expire (80+50=130 > 100)
    ASSERT_TRUE(dict_.empty());
    ASSERT_EQ(dict_.size(), 0);
}

TEST_F(ExpiringDictTest, Clear) {
    dict_.insert("one", 1);
    dict_.insert("two", 2);
    ASSERT_EQ(dict_.size(), 2);
    dict_.clear();
    ASSERT_TRUE(dict_.empty());
    ASSERT_EQ(dict_.size(), 0);
}

TEST_F(ExpiringDictTest, Update) {
    dict_.insert("one", 1);
    ASSERT_TRUE(dict_.update("one", 11));
    int* val = dict_.find("one");
    ASSERT_NE(val, nullptr);
    ASSERT_EQ(*val, 11);

    ASSERT_FALSE(dict_.update("two", 22)); // "two" does not exist, should insert
    ASSERT_TRUE(dict_.contains("two"));
    val = dict_.find("two");
    ASSERT_NE(val, nullptr);
    ASSERT_EQ(*val, 22);
}

TEST_F(ExpiringDictTest, UpdateRValue) {
    dict_.insert("one", 1);
    int new_val = 111;
    ASSERT_TRUE(dict_.update("one", std::move(new_val)));
    int* val = dict_.find("one");
    ASSERT_NE(val, nullptr);
    ASSERT_EQ(*val, 111);

    int newer_val = 222;
    ASSERT_FALSE(dict_.update("two", std::move(newer_val))); // "two" does not exist, should insert
    ASSERT_TRUE(dict_.contains("two"));
    val = dict_.find("two");
    ASSERT_NE(val, nullptr);
    ASSERT_EQ(*val, 222);
}


TEST_F(ExpiringDictTest, SetAndGetTtl) {
    dict_.set_ttl(200ms);
    ASSERT_EQ(dict_.get_ttl(), 200ms);

    dict_.insert("test_ttl", 100);
    sleep_for_ms(150);
    ASSERT_TRUE(dict_.contains("test_ttl")); // Not expired yet

    sleep_for_ms(100); // Total 250ms, should expire
    ASSERT_FALSE(dict_.contains("test_ttl"));
}

TEST_F(ExpiringDictTest, AccessRenewsTtl_False) {
    // Default setup is access_renews = false
    dict_.insert("no_renew", 1);
    sleep_for_ms(80);
    ASSERT_NE(dict_.find("no_renew"), nullptr); // Access it
    ASSERT_TRUE(dict_.contains("no_renew"));    // Access it via contains

    sleep_for_ms(50); // Total 80+50=130ms. Should expire as access doesn't renew
    ASSERT_FALSE(dict_.contains("no_renew"));
}

TEST_F(ExpiringDictTest, AccessRenewsTtl_True_Find) {
    dict_.set_access_renews(true);
    ASSERT_TRUE(dict_.get_access_renews());
    dict_.insert("renew_find", 1);

    for (int i = 0; i < 3; ++i) {
        sleep_for_ms(80);
        ASSERT_NE(dict_.find("renew_find"), nullptr) << "Failed on iteration " << i; // Access and renew
    }
    sleep_for_ms(80); // Still should be there
    ASSERT_TRUE(dict_.contains("renew_find"));

    sleep_for_ms(120); // Now let it expire
    ASSERT_FALSE(dict_.contains("renew_find"));
}

TEST_F(ExpiringDictTest, AccessRenewsTtl_True_Contains) {
    dict_.set_access_renews(true);
    dict_.insert("renew_contains", 1);

    for (int i = 0; i < 3; ++i) {
        sleep_for_ms(80);
        ASSERT_TRUE(dict_.contains("renew_contains")) << "Failed on iteration " << i; // Access and renew
    }
    sleep_for_ms(80); // Still should be there
    ASSERT_TRUE(dict_.contains("renew_contains"));

    sleep_for_ms(120); // Now let it expire
    ASSERT_FALSE(dict_.contains("renew_contains"));
}

TEST_F(ExpiringDictTest, ForEach) {
    dict_.insert("a", 1);
    dict_.insert("b", 2);
    dict_.insert("c", 3);

    std::unordered_map<std::string, int> collected;
    dict_.for_each([&](const std::string& k, const int& v) {
        collected[k] = v;
    });

    ASSERT_EQ(collected.size(), 3);
    ASSERT_EQ(collected["a"], 1);
    ASSERT_EQ(collected["b"], 2);
    ASSERT_EQ(collected["c"], 3);

    sleep_for_ms(120); // Expire all
    collected.clear();
    dict_.for_each([&](const std::string& k, const int& v) {
        collected[k] = v;
    });
    ASSERT_TRUE(collected.empty());
}

TEST_F(ExpiringDictTest, FindConstAfterExpiration) {
    dict_.insert("one", 1);
    sleep_for_ms(120); // Expire "one"

    const auto& const_dict = dict_;
    ASSERT_EQ(const_dict.find("one"), nullptr); // Should return nullptr, not erase

    // To verify it wasn't erased by const find, try non-const find (which erases)
    ASSERT_EQ(dict_.find("one"), nullptr);
    // And check size after non-const find implicitly calls expire
    ASSERT_EQ(dict_.size(),0);
}

TEST_F(ExpiringDictTest, ExpireMethodDirectCall) {
    dict_.insert("a", 1);
    dict_.insert("b", 2);
    sleep_for_ms(120); // Both should be expired
    dict_.expire();   // Manually call expire
    ASSERT_TRUE(dict_.empty());
}
