#include "gtest/gtest.h"
#include "../include/predicate_cache.h" // Adjust path as necessary
#include <string>
#include <atomic> // For std::atomic_int
#include <vector> // For std::vector in tests
#include <stdexcept> // For std::out_of_range

// A simple struct for testing
struct MyObject {
    int id;
    std::string data;
    mutable int evaluation_count; // To track actual predicate calls for some tests

    MyObject(int i = 0, std::string d = "", int eval_count = 0) : id(i), data(std::move(d)), evaluation_count(eval_count) {}

    // Equality operator for std::unordered_map and EXPECT_EQ
    bool operator==(const MyObject& other) const {
        return id == other.id && data == other.data;
    }
};

// Hash function for MyObject for std::unordered_map
namespace std {
    template <>
    struct hash<MyObject> {
        size_t operator()(const MyObject& obj) const {
            size_t h1 = std::hash<int>()(obj.id);
            size_t h2 = std::hash<std::string>()(obj.data);
            return h1 ^ (h2 << 1); // Combine hashes
        }
    };
}

// Test fixture for PredicateCache tests
class PredicateCacheTest : public ::testing::Test {
protected:
    PredicateCache<MyObject> pc;
    MyObject obj1{1, "hello"};
    MyObject obj2{2, "world"};
    MyObject obj3{3, ""}; // Empty data

    // Predicate examples
    static bool isEven(const MyObject& obj) {
        obj.evaluation_count++;
        return obj.id % 2 == 0;
    }

    static bool hasNonEmptyData(const MyObject& obj) {
        obj.evaluation_count++;
        return !obj.data.empty();
    }

    static bool isIdGreaterThanTen(const MyObject& obj) {
        obj.evaluation_count++;
        return obj.id > 10;
    }

    PredicateId isEvenId;
    PredicateId hasNonEmptyDataId;
    PredicateId isIdGreaterThanTenId;

    void SetUp() override {
        // Reset evaluation counts for objects that might be reused across test cases if they were global
        // For member objects, they are fresh per test case of the fixture.
        obj1.evaluation_count = 0;
        obj2.evaluation_count = 0;
        obj3.evaluation_count = 0;

        // Register predicates
        isEvenId = pc.register_predicate(isEven);
        hasNonEmptyDataId = pc.register_predicate(hasNonEmptyData);
        isIdGreaterThanTenId = pc.register_predicate(isIdGreaterThanTen);
    }
};


TEST_F(PredicateCacheTest, RegistrationAndIds) {
    EXPECT_EQ(isEvenId, 0);
    EXPECT_EQ(hasNonEmptyDataId, 1);
    EXPECT_EQ(isIdGreaterThanTenId, 2);

    PredicateId id4 = pc.register_predicate([](const MyObject&){ return true; });
    EXPECT_EQ(id4, 3);
}

TEST_F(PredicateCacheTest, EvaluateAndCaching) {
    obj1.evaluation_count = 0;
    EXPECT_FALSE(pc.evaluate(obj1, isEvenId)); // 1 is not even
    EXPECT_EQ(obj1.evaluation_count, 1);

    EXPECT_FALSE(pc.evaluate(obj1, isEvenId)); // Should be cached
    EXPECT_EQ(obj1.evaluation_count, 1);       // Not called again

    obj2.evaluation_count = 0;
    EXPECT_TRUE(pc.evaluate(obj2, isEvenId));  // 2 is even
    EXPECT_EQ(obj2.evaluation_count, 1);

    EXPECT_TRUE(pc.evaluate(obj2, isEvenId));  // Should be cached
    EXPECT_EQ(obj2.evaluation_count, 1);       // Not called again
}

TEST_F(PredicateCacheTest, GetIf) {
    // Before evaluation
    EXPECT_FALSE(pc.get_if(obj1, isEvenId).has_value());
    EXPECT_FALSE(pc.get_if(obj1, hasNonEmptyDataId).has_value());

    // Evaluate one predicate
    pc.evaluate(obj1, isEvenId); // Result is false

    ASSERT_TRUE(pc.get_if(obj1, isEvenId).has_value());
    EXPECT_FALSE(pc.get_if(obj1, isEvenId).value());
    EXPECT_FALSE(pc.get_if(obj1, hasNonEmptyDataId).has_value()); // Other predicate still not cached

    // Try to get_if for an object not in cache at all
    MyObject obj_not_seen{100, "not seen"};
    EXPECT_FALSE(pc.get_if(obj_not_seen, isEvenId).has_value());

    // Try to get_if for an invalid predicate ID
    ASSERT_THROW(pc.get_if(obj1, 999), std::out_of_range);
}

TEST_F(PredicateCacheTest, Prime) {
    obj1.evaluation_count = 0;

    // Prime the cache for obj1, isEvenId
    pc.prime(obj1, isEvenId, true); // Manually set to true, even though 1 is not even
    EXPECT_EQ(obj1.evaluation_count, 0); // Predicate should not be called

    // Evaluate should return the primed value
    EXPECT_TRUE(pc.evaluate(obj1, isEvenId));
    EXPECT_EQ(obj1.evaluation_count, 0); // Still not called

    // get_if should also return the primed value
    ASSERT_TRUE(pc.get_if(obj1, isEvenId).has_value());
    EXPECT_TRUE(pc.get_if(obj1, isEvenId).value());

    // Prime with a different value
    pc.prime(obj1, isEvenId, false);
    EXPECT_EQ(obj1.evaluation_count, 0);
    EXPECT_FALSE(pc.evaluate(obj1, isEvenId));
    EXPECT_EQ(obj1.evaluation_count, 0);

    // Test priming for an invalid predicate ID
    ASSERT_THROW(pc.prime(obj1, 999, true), std::out_of_range);
}

TEST_F(PredicateCacheTest, InvalidateObject) {
    // Evaluate for obj1
    obj1.evaluation_count = 0;
    pc.evaluate(obj1, isEvenId); // false
    pc.evaluate(obj1, hasNonEmptyDataId); // true
    EXPECT_EQ(obj1.evaluation_count, 2);

    obj2.evaluation_count = 0;
    pc.evaluate(obj2, isEvenId); // true
    EXPECT_EQ(obj2.evaluation_count, 1);

    // Invalidate obj1
    pc.invalidate(obj1);

    // Check obj1: get_if should return nullopt
    EXPECT_FALSE(pc.get_if(obj1, isEvenId).has_value());
    EXPECT_FALSE(pc.get_if(obj1, hasNonEmptyDataId).has_value());

    // Re-evaluating obj1 should call predicates again
    obj1.evaluation_count = 0; // Reset for re-evaluation
    EXPECT_FALSE(pc.evaluate(obj1, isEvenId));
    EXPECT_EQ(obj1.evaluation_count, 1);
    EXPECT_TRUE(pc.evaluate(obj1, hasNonEmptyDataId));
    EXPECT_EQ(obj1.evaluation_count, 2);

    // obj2 should still be cached
    obj2.evaluation_count = 0; // Reset to ensure it's not recounted
    EXPECT_TRUE(pc.evaluate(obj2, isEvenId));
    EXPECT_EQ(obj2.evaluation_count, 0); // Not called again for obj2

    // Invalidate an object not in cache - should be a no-op
    MyObject obj_not_cached{4, "not_cached_yet"};
    ASSERT_NO_THROW(pc.invalidate(obj_not_cached));
}

TEST_F(PredicateCacheTest, InvalidateAll) {
    obj1.evaluation_count = 0;
    obj2.evaluation_count = 0;
    pc.evaluate(obj1, isEvenId); // false
    pc.evaluate(obj2, isEvenId); // true
    EXPECT_EQ(obj1.evaluation_count + obj2.evaluation_count, 2);

    pc.invalidate_all();

    // Check obj1 and obj2: get_if should return nullopt
    EXPECT_FALSE(pc.get_if(obj1, isEvenId).has_value());
    EXPECT_FALSE(pc.get_if(obj2, isEvenId).has_value());

    // Re-evaluating should call predicates again
    obj1.evaluation_count = 0;
    obj2.evaluation_count = 0;
    EXPECT_FALSE(pc.evaluate(obj1, isEvenId));
    EXPECT_EQ(obj1.evaluation_count, 1);
    EXPECT_TRUE(pc.evaluate(obj2, isEvenId));
    EXPECT_EQ(obj2.evaluation_count, 1);
}

TEST_F(PredicateCacheTest, RemoveObject) {
    obj1.evaluation_count = 0;
    obj2.evaluation_count = 0;
    pc.evaluate(obj1, isEvenId);
    pc.evaluate(obj2, isEvenId);
    ASSERT_EQ(pc.size(), 2);
    EXPECT_EQ(obj1.evaluation_count + obj2.evaluation_count, 2);

    pc.remove(obj1);
    ASSERT_EQ(pc.size(), 1);
    EXPECT_FALSE(pc.get_if(obj1, isEvenId).has_value()); // obj1 should be gone

    // Re-evaluating obj1 should treat it as new
    obj1.evaluation_count = 0;
    EXPECT_FALSE(pc.evaluate(obj1, isEvenId));
    EXPECT_EQ(obj1.evaluation_count, 1); // Called again for obj1
    ASSERT_EQ(pc.size(), 2); // obj1 is back

    // obj2 should still be cached
    obj2.evaluation_count = 0; // Reset to ensure it's not recounted
    EXPECT_TRUE(pc.evaluate(obj2, isEvenId));
    EXPECT_EQ(obj2.evaluation_count, 0); // Not called again for obj2

    // Remove an object not in cache - should be a no-op and not change size
    MyObject obj_not_present{4, "not_present"};
    pc.remove(obj_not_present);
    ASSERT_EQ(pc.size(), 2); // Size remains 2 (obj1 and obj2)
}

TEST_F(PredicateCacheTest, MultiplePredicatesAndSize) {
    MyObject objA{10, "dataA"};
    MyObject objB{11, ""};      // Empty data
    MyObject objC{12, "dataC"};

    ASSERT_EQ(pc.size(), 0); // Cache is initially empty (before these objects are added)

    objA.evaluation_count = 0;
    objB.evaluation_count = 0;
    objC.evaluation_count = 0;
    int initial_objA_eval_count = 0;
    int initial_objB_eval_count = 0;


    // ObjA
    EXPECT_TRUE(pc.evaluate(objA, isEvenId));    // 10 is even
    initial_objA_eval_count++;
    EXPECT_TRUE(pc.evaluate(objA, hasNonEmptyDataId));   // "dataA" is non-empty
    initial_objA_eval_count++;
    EXPECT_FALSE(pc.evaluate(objA, isIdGreaterThanTenId)); // 10 is not > 10
    initial_objA_eval_count++;
    EXPECT_EQ(objA.evaluation_count, initial_objA_eval_count);
    ASSERT_EQ(pc.size(), 1);

    // ObjB
    EXPECT_FALSE(pc.evaluate(objB, isEvenId));   // 11 is not even
    initial_objB_eval_count++;
    EXPECT_FALSE(pc.evaluate(objB, hasNonEmptyDataId));  // "" is empty
    initial_objB_eval_count++;
    EXPECT_TRUE(pc.evaluate(objB, isIdGreaterThanTenId)); // 11 is > 10
    initial_objB_eval_count++;
    EXPECT_EQ(objB.evaluation_count, initial_objB_eval_count);
    ASSERT_EQ(pc.size(), 2);

    // Check caching for ObjA again for one predicate
    EXPECT_TRUE(pc.evaluate(objA, hasNonEmptyDataId));
    EXPECT_EQ(objA.evaluation_count, initial_objA_eval_count); // Should not increment

    // ObjC (only evaluate one predicate)
    int initial_objC_eval_count = 0;
    EXPECT_TRUE(pc.evaluate(objC, isEvenId));    // 12 is even
    initial_objC_eval_count++;
    EXPECT_EQ(objC.evaluation_count, initial_objC_eval_count);
    ASSERT_EQ(pc.size(), 3);

    // Check get_if for not-yet-evaluated predicate on ObjC
    EXPECT_FALSE(pc.get_if(objC, hasNonEmptyDataId).has_value());

    // Evaluate it now
    EXPECT_TRUE(pc.evaluate(objC, hasNonEmptyDataId)); // "dataC" is non-empty
    initial_objC_eval_count++;
    EXPECT_EQ(objC.evaluation_count, initial_objC_eval_count);
    ASSERT_TRUE(pc.get_if(objC, hasNonEmptyDataId).has_value());
    EXPECT_TRUE(pc.get_if(objC, hasNonEmptyDataId).value());
}

TEST_F(PredicateCacheTest, EvaluateThrowsForInvalidId) {
    ASSERT_THROW(pc.evaluate(obj1, 999), std::out_of_range);
}

TEST_F(PredicateCacheTest, SizeReporting) {
    EXPECT_EQ(pc.size(), 0);
    pc.evaluate(obj1, isEvenId);
    EXPECT_EQ(pc.size(), 1);
    pc.evaluate(obj2, isEvenId);
    EXPECT_EQ(pc.size(), 2);
    pc.evaluate(obj1, hasNonEmptyDataId); // Same object, different predicate
    EXPECT_EQ(pc.size(), 2); // Size is number of objects
    pc.remove(obj1);
    EXPECT_EQ(pc.size(), 1);
    pc.invalidate_all(); // Does not change object count, only result status
    EXPECT_EQ(pc.size(), 1); // obj2 is still "known"
    pc.remove(obj2);
    EXPECT_EQ(pc.size(), 0);
}
// Note: The main function is not needed here as GTest provides its own.
// The tests/CMakeLists.txt links against GTest::gtest_main.
