#include "gtest/gtest.h"
#include "../include/disjoint_set_union.h" // Adjust path as needed
#include <string>
#include <vector>
#include <set> // For comparing sets of elements
#include <algorithm> // For std::sort, std::all_of
#include <map> // For getAllSets verification
#include <chrono> // For performance tests
#include <iostream> // For printing performance info

// Helper for custom struct tests
struct CustomData {
    int id;
    std::string name;

    bool operator==(const CustomData& other) const {
        return id == other.id && name == other.name;
    }
    // Required for std::set or std::map keys if not providing custom comparator
    bool operator<(const CustomData& other) const {
        if (id != other.id) {
            return id < other.id;
        }
        return name < other.name;
    }

    // Inequality operator
    bool operator!=(const CustomData& other) const {
        return !(*this == other);
    }
};

// Hash function for CustomData for DisjointSetUnion<CustomData>
namespace std {
    template <>
    struct hash<CustomData> {
        size_t operator()(const CustomData& cd) const {
            size_t h1 = hash<int>()(cd.id);
            size_t h2 = hash<string>()(cd.name);
            return h1 ^ (h2 << 1); 
        }
    };
}

// Test fixture for DisjointSetUnion<T>
template <typename T>
class DSTest : public ::testing::Test {
protected:
    DisjointSetUnion<T> dsu;

public: // Made public static for use in TYPED_TEST macros
    static T CreateVal(int unique_id, const std::string& name_prefix = "name") {
        if constexpr (std::is_same_v<T, int>) {
            return unique_id;
        } else if constexpr (std::is_same_v<T, std::string>) {
            return name_prefix + std::to_string(unique_id);
        } else if constexpr (std::is_same_v<T, CustomData>) {
            return {unique_id, name_prefix + std::to_string(unique_id)};
        }
        // Should not happen with MyTypes restriction but as a fallback:
        return T(); 
    }
    
    // Helper function to convert vector to set for easier comparison of contents (ignoring order)
    // Made static as it doesn't depend on fixture instance members and is used by normalizeSets.
    static std::set<T> toSet(const std::vector<T>& vec) {
        return std::set<T>(vec.begin(), vec.end());
    }

    // Helper to convert vector of vectors to a comparable form (set of sets)
    // Made static as it uses static toSet and doesn't depend on instance members.
    static std::set<std::set<T>> normalizeSets(const std::vector<std::vector<T>>& setsVec) {
        std::set<std::set<T>> normalized;
        for (const auto& setVec_inner : setsVec) {
            // Use DSTest<T>::toSet if TypeParam is needed, but T is template param of class
            // so it's fine as is. For clarity, could use DSTest<T>::toSet or self::toSet
            normalized.insert(toSet(setVec_inner)); 
        }
        return normalized;
    }
};

using MyTypes = ::testing::Types<int, std::string, CustomData>;
TYPED_TEST_SUITE(DSTest, MyTypes);

TYPED_TEST(DSTest, InitialState) {
    EXPECT_EQ(this->dsu.countSets(), 0);
    EXPECT_EQ(this->dsu.totalElements(), 0);
    EXPECT_TRUE(this->dsu.isEmpty());
}

TYPED_TEST(DSTest, MakeSet) {
    TypeParam val1 = DSTest<TypeParam>::CreateVal(1, "Alice");
    this->dsu.makeSet(val1);
    EXPECT_EQ(this->dsu.countSets(), 1);
    EXPECT_EQ(this->dsu.totalElements(), 1);
    EXPECT_FALSE(this->dsu.isEmpty());
    EXPECT_TRUE(this->dsu.contains(val1));
    EXPECT_EQ(this->dsu.find(val1), val1);
    EXPECT_EQ(this->dsu.size(val1), 1);

    this->dsu.makeSet(val1); // Make set on existing element
    EXPECT_EQ(this->dsu.countSets(), 1);
    EXPECT_EQ(this->dsu.totalElements(), 1);

    TypeParam val2 = DSTest<TypeParam>::CreateVal(2, "Bob");
    this->dsu.makeSet(val2);
    EXPECT_EQ(this->dsu.countSets(), 2);
    EXPECT_EQ(this->dsu.totalElements(), 2);
    EXPECT_TRUE(this->dsu.contains(val2));
    EXPECT_NE(this->dsu.find(val1), this->dsu.find(val2));
}

TYPED_TEST(DSTest, FindOperationAutoCreates) {
    TypeParam val1 = DSTest<TypeParam>::CreateVal(10, "FindTest1");
    EXPECT_EQ(this->dsu.find(val1), val1); // Auto-creates
    EXPECT_EQ(this->dsu.countSets(), 1);
    EXPECT_EQ(this->dsu.totalElements(), 1);
    EXPECT_TRUE(this->dsu.contains(val1));
    EXPECT_EQ(this->dsu.size(val1), 1);

    TypeParam val2 = DSTest<TypeParam>::CreateVal(20, "FindTest2");
    EXPECT_EQ(this->dsu.find(val2), val2); // Auto-creates another
    EXPECT_EQ(this->dsu.countSets(), 2);
    EXPECT_EQ(this->dsu.totalElements(), 2);
    EXPECT_NE(this->dsu.find(val1), this->dsu.find(val2));
}

TYPED_TEST(DSTest, UnionSetsSimple) {
    TypeParam val1 = DSTest<TypeParam>::CreateVal(1, "UAlice");
    TypeParam val2 = DSTest<TypeParam>::CreateVal(2, "UBob");
    TypeParam val3 = DSTest<TypeParam>::CreateVal(3, "UCharlie");
    this->dsu.makeSet(val1); this->dsu.makeSet(val2); this->dsu.makeSet(val3);
    EXPECT_EQ(this->dsu.countSets(), 3);

    EXPECT_TRUE(this->dsu.unionSets(val1, val2));
    EXPECT_EQ(this->dsu.countSets(), 2);
    EXPECT_TRUE(this->dsu.connected(val1, val2));
    EXPECT_FALSE(this->dsu.connected(val1, val3));
    EXPECT_EQ(this->dsu.size(val1), 2);
    EXPECT_EQ(this->dsu.size(val2), 2);
    EXPECT_EQ(this->dsu.find(val1), this->dsu.find(val2));

    EXPECT_FALSE(this->dsu.unionSets(val1, val2)); // Already connected
    EXPECT_EQ(this->dsu.countSets(), 2);

    EXPECT_TRUE(this->dsu.unionSets(val1, val3));
    EXPECT_EQ(this->dsu.countSets(), 1);
    EXPECT_TRUE(this->dsu.connected(val1, val3));
    EXPECT_TRUE(this->dsu.connected(val2, val3));
    EXPECT_EQ(this->dsu.size(val1), 3);
    TypeParam finalRoot = this->dsu.find(val1);
    EXPECT_EQ(this->dsu.find(val2), finalRoot);
    EXPECT_EQ(this->dsu.find(val3), finalRoot);
}

TYPED_TEST(DSTest, GetAllSets) {
    EXPECT_TRUE(this->dsu.getAllSets().empty()); // Empty DSU

    TypeParam v1 = DSTest<TypeParam>::CreateVal(1, "S1");
    TypeParam v2 = DSTest<TypeParam>::CreateVal(2, "S2");
    TypeParam v3 = DSTest<TypeParam>::CreateVal(3, "S3");
    TypeParam v4 = DSTest<TypeParam>::CreateVal(4, "S4");

    this->dsu.makeSet(v1);
    this->dsu.makeSet(v2);
    this->dsu.makeSet(v3);
    this->dsu.makeSet(v4);

    auto sets1_expected = DSTest<TypeParam>::normalizeSets({{v1}, {v2}, {v3}, {v4}});
    EXPECT_EQ(DSTest<TypeParam>::normalizeSets(this->dsu.getAllSets()), sets1_expected);

    this->dsu.unionSets(v1, v2);
    auto sets2_expected = DSTest<TypeParam>::normalizeSets({{v1, v2}, {v3}, {v4}});
    EXPECT_EQ(DSTest<TypeParam>::normalizeSets(this->dsu.getAllSets()), sets2_expected);
    
    this->dsu.unionSets(v3, v4);
    auto sets3_expected = DSTest<TypeParam>::normalizeSets({{v1, v2}, {v3, v4}});
    EXPECT_EQ(DSTest<TypeParam>::normalizeSets(this->dsu.getAllSets()), sets3_expected);

    this->dsu.unionSets(v1, v4); 
    auto sets4_expected = DSTest<TypeParam>::normalizeSets({{v1, v2, v3, v4}});
    EXPECT_EQ(DSTest<TypeParam>::normalizeSets(this->dsu.getAllSets()), sets4_expected);
    
    this->dsu.clear();
    EXPECT_TRUE(this->dsu.getAllSets().empty()); 
}

TYPED_TEST(DSTest, ResetOperation) {
    TypeParam v1 = DSTest<TypeParam>::CreateVal(1, "R1");
    TypeParam v2 = DSTest<TypeParam>::CreateVal(2, "R2");
    TypeParam v3 = DSTest<TypeParam>::CreateVal(3, "R3");
    this->dsu.makeSet(v1); this->dsu.makeSet(v2); this->dsu.makeSet(v3);
    this->dsu.unionSets(v1, v2);
    ASSERT_EQ(this->dsu.countSets(), 2);
    ASSERT_EQ(this->dsu.totalElements(), 3);

    this->dsu.reset();
    EXPECT_EQ(this->dsu.countSets(), 3);
    EXPECT_EQ(this->dsu.totalElements(), 3); 
    EXPECT_FALSE(this->dsu.isEmpty());

    EXPECT_EQ(this->dsu.find(v1), v1);
    EXPECT_EQ(this->dsu.find(v2), v2);
    EXPECT_EQ(this->dsu.find(v3), v3);
    EXPECT_EQ(this->dsu.size(v1), 1);
    EXPECT_EQ(this->dsu.size(v2), 1);
    EXPECT_EQ(this->dsu.size(v3), 1);
    EXPECT_FALSE(this->dsu.connected(v1, v2));
    EXPECT_TRUE(this->dsu.contains(v1)); 
}

TYPED_TEST(DSTest, CompressOperation) {
    TypeParam v1 = DSTest<TypeParam>::CreateVal(1, "C1");
    TypeParam v2 = DSTest<TypeParam>::CreateVal(2, "C2");
    TypeParam v3 = DSTest<TypeParam>::CreateVal(3, "C3");
    this->dsu.makeSet(v1); this->dsu.makeSet(v2); this->dsu.makeSet(v3);
    this->dsu.unionSets(v1, v2);
    this->dsu.unionSets(v2, v3); 

    this->dsu.compress();
    
    TypeParam root = this->dsu.find(v3); 
    EXPECT_EQ(this->dsu.find(v1), root); 
    EXPECT_EQ(this->dsu.find(v2), root); 
    EXPECT_TRUE(this->dsu.connected(v1, v3));
    EXPECT_EQ(this->dsu.size(v1), 3);
}

TYPED_TEST(DSTest, IsEmptyAdvanced) {
    EXPECT_TRUE(this->dsu.isEmpty());
    TypeParam v1 = DSTest<TypeParam>::CreateVal(1, "E1");
    this->dsu.makeSet(v1);
    EXPECT_FALSE(this->dsu.isEmpty());
    this->dsu.reset(); 
    EXPECT_FALSE(this->dsu.isEmpty());
    this->dsu.clear();
    EXPECT_TRUE(this->dsu.isEmpty());
}

TYPED_TEST(DSTest, ContainsAdvanced) {
    TypeParam v1 = DSTest<TypeParam>::CreateVal(1, "CA1");
    TypeParam v_non_existent = DSTest<TypeParam>::CreateVal(99, "NonExistent");
    EXPECT_FALSE(this->dsu.contains(v1));
    this->dsu.makeSet(v1);
    EXPECT_TRUE(this->dsu.contains(v1));
    EXPECT_FALSE(this->dsu.contains(v_non_existent));

    this->dsu.reset();
    EXPECT_TRUE(this->dsu.contains(v1)); 

    this->dsu.clear();
    EXPECT_FALSE(this->dsu.contains(v1));
    EXPECT_FALSE(this->dsu.contains(v_non_existent));
}

TYPED_TEST(DSTest, ClearOperation) {
    TypeParam v1 = DSTest<TypeParam>::CreateVal(1, "CL1");
    TypeParam v2 = DSTest<TypeParam>::CreateVal(2, "CL2");
    this->dsu.makeSet(v1); this->dsu.makeSet(v2);
    this->dsu.unionSets(v1, v2);
    ASSERT_EQ(this->dsu.countSets(), 1);
    ASSERT_EQ(this->dsu.totalElements(), 2);

    this->dsu.clear();
    EXPECT_EQ(this->dsu.countSets(), 0);
    EXPECT_EQ(this->dsu.totalElements(), 0);
    EXPECT_TRUE(this->dsu.isEmpty());
    EXPECT_FALSE(this->dsu.contains(v1));

    EXPECT_EQ(this->dsu.find(v1), v1); // Auto-creates
    EXPECT_EQ(this->dsu.countSets(), 1);
    EXPECT_EQ(this->dsu.totalElements(), 1);
    EXPECT_TRUE(this->dsu.contains(v1));
}

TYPED_TEST(DSTest, GetSetMembers) {
    TypeParam v1 = DSTest<TypeParam>::CreateVal(1, "M1");
    TypeParam v2 = DSTest<TypeParam>::CreateVal(2, "M2");
    TypeParam v3 = DSTest<TypeParam>::CreateVal(3, "M3");
    TypeParam v4 = DSTest<TypeParam>::CreateVal(4, "M4_Auto");

    this->dsu.makeSet(v1); this->dsu.makeSet(v2); this->dsu.makeSet(v3);
    this->dsu.unionSets(v1, v2);

    std::set<TypeParam> set1_members_expected = {v1, v2};
    EXPECT_EQ(DSTest<TypeParam>::toSet(this->dsu.getSetMembers(v1)), set1_members_expected);
    EXPECT_EQ(DSTest<TypeParam>::toSet(this->dsu.getSetMembers(v2)), set1_members_expected);

    std::set<TypeParam> set3_members_expected = {v3};
    EXPECT_EQ(DSTest<TypeParam>::toSet(this->dsu.getSetMembers(v3)), set3_members_expected);
    
    std::set<TypeParam> set4_members_expected = {v4};
    EXPECT_EQ(DSTest<TypeParam>::toSet(this->dsu.getSetMembers(v4)), set4_members_expected); // Auto-creates
    EXPECT_TRUE(this->dsu.contains(v4));
    EXPECT_EQ(this->dsu.countSets(), 3); 
}

TYPED_TEST(DSTest, PathCompressionChain) {
    TypeParam v1 = DSTest<TypeParam>::CreateVal(1, "PC1");
    TypeParam v2 = DSTest<TypeParam>::CreateVal(2, "PC2");
    TypeParam v3 = DSTest<TypeParam>::CreateVal(3, "PC3");
    TypeParam v4 = DSTest<TypeParam>::CreateVal(4, "PC4");
    TypeParam v5 = DSTest<TypeParam>::CreateVal(5, "PC5");

    this->dsu.makeSet(v1); this->dsu.makeSet(v2); this->dsu.makeSet(v3);
    this->dsu.makeSet(v4); this->dsu.makeSet(v5);

    this->dsu.unionSets(v1,v2); this->dsu.unionSets(v2,v3);
    this->dsu.unionSets(v3,v4); this->dsu.unionSets(v4,v5); 

    TypeParam root = this->dsu.find(v5); 
    EXPECT_EQ(this->dsu.find(v1), root);
    EXPECT_EQ(this->dsu.find(v2), root);
    EXPECT_EQ(this->dsu.find(v3), root);
    EXPECT_EQ(this->dsu.find(v4), root);
    
    EXPECT_TRUE(this->dsu.connected(v1,v5));
    EXPECT_EQ(this->dsu.size(v1), 5);
}

TEST(DSUCustomDataTest, BasicOperations) {
    DisjointSetUnion<CustomData> dsu;
    CustomData cd1 = {1, "Alice"};
    CustomData cd2 = {2, "Bob"};
    dsu.makeSet(cd1); dsu.makeSet(cd2);
    EXPECT_EQ(dsu.countSets(), 2);
    EXPECT_TRUE(dsu.unionSets(cd1, cd2));
    EXPECT_TRUE(dsu.connected(cd1, cd2));
    EXPECT_EQ(dsu.size(cd1), 2);
}

// =====================================================================================
// Tests for FastDSU
// =====================================================================================

class FastDSUTest : public ::testing::Test {
protected:
    // Helper to convert vector of vectors to a comparable form (set of sets)
    // For FastDSU, elements are always int.
    // This can be static as it doesn't depend on FastDSUTest instance members.
    static std::set<std::set<int>> normalizeFastDSUSets(const std::vector<std::vector<int>>& setsVec) {
        std::set<std::set<int>> normalized;
        for (const auto& setVec : setsVec) {
            normalized.insert(std::set<int>(setVec.begin(), setVec.end()));
        }
        return normalized;
    }
};

TEST_F(FastDSUTest, InitialState) {
    FastDSU dsu_10(10);
    EXPECT_EQ(dsu_10.countSets(), 10);
    EXPECT_FALSE(dsu_10.isEmpty());
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(dsu_10.contains(i));
        EXPECT_EQ(dsu_10.find(i), i);
        EXPECT_EQ(dsu_10.size(i), 1);
    }
    EXPECT_FALSE(dsu_10.contains(10)); 
    EXPECT_FALSE(dsu_10.contains(-1));  

    FastDSU dsu_0(0);
    EXPECT_EQ(dsu_0.countSets(), 0);
    EXPECT_TRUE(dsu_0.isEmpty());
    EXPECT_FALSE(dsu_0.contains(0));
}

TEST_F(FastDSUTest, MakeSetNoOp) {
    FastDSU dsu_5(5);
    dsu_5.makeSet(0); 
    dsu_5.makeSet(4);
    EXPECT_EQ(dsu_5.countSets(), 5); 
    EXPECT_EQ(dsu_5.find(0), 0);
    EXPECT_EQ(dsu_5.size(0), 1);
}

TEST_F(FastDSUTest, UnionSetsSimple) {
    FastDSU dsu_5(5);
    EXPECT_EQ(dsu_5.countSets(), 5);

    EXPECT_TRUE(dsu_5.unionSets(0, 1));
    EXPECT_EQ(dsu_5.countSets(), 4);
    EXPECT_TRUE(dsu_5.connected(0, 1));
    EXPECT_FALSE(dsu_5.connected(0, 2));
    EXPECT_EQ(dsu_5.size(0), 2);
    EXPECT_EQ(dsu_5.size(1), 2);
    EXPECT_EQ(dsu_5.find(0), dsu_5.find(1));

    EXPECT_FALSE(dsu_5.unionSets(0, 1)); 
    EXPECT_EQ(dsu_5.countSets(), 4);

    EXPECT_TRUE(dsu_5.unionSets(0, 2)); 
    EXPECT_EQ(dsu_5.countSets(), 3);
    EXPECT_TRUE(dsu_5.connected(0, 2));
    EXPECT_TRUE(dsu_5.connected(1, 2));
    EXPECT_EQ(dsu_5.size(0), 3);
    EXPECT_EQ(dsu_5.size(1), 3);
    EXPECT_EQ(dsu_5.size(2), 3);
    int root = dsu_5.find(0);
    EXPECT_EQ(dsu_5.find(1), root);
    EXPECT_EQ(dsu_5.find(2), root);
}

TEST_F(FastDSUTest, GetAllSets) {
    FastDSU dsu_0(0);
    EXPECT_TRUE(dsu_0.getAllSets().empty());

    FastDSU dsu_4(4);
    auto sets1_expected = FastDSUTest::normalizeFastDSUSets({{0}, {1}, {2}, {3}});
    EXPECT_EQ(FastDSUTest::normalizeFastDSUSets(dsu_4.getAllSets()), sets1_expected);

    dsu_4.unionSets(0, 1);
    auto sets2_expected = FastDSUTest::normalizeFastDSUSets({{0, 1}, {2}, {3}});
    EXPECT_EQ(FastDSUTest::normalizeFastDSUSets(dsu_4.getAllSets()), sets2_expected);
    
    dsu_4.unionSets(2, 3);
    auto sets3_expected = FastDSUTest::normalizeFastDSUSets({{0, 1}, {2, 3}});
    EXPECT_EQ(FastDSUTest::normalizeFastDSUSets(dsu_4.getAllSets()), sets3_expected);

    dsu_4.unionSets(0, 3); 
    auto sets4_expected = FastDSUTest::normalizeFastDSUSets({{0, 1, 2, 3}});
    EXPECT_EQ(FastDSUTest::normalizeFastDSUSets(dsu_4.getAllSets()), sets4_expected);
}

TEST_F(FastDSUTest, ResetOperation) {
    FastDSU dsu_3(3);
    dsu_3.unionSets(0,1);
    dsu_3.unionSets(1,2); 
    ASSERT_EQ(dsu_3.countSets(), 1);

    dsu_3.reset();
    EXPECT_EQ(dsu_3.countSets(), 3);
    EXPECT_FALSE(dsu_3.isEmpty());
    for (int i=0; i<3; ++i) {
        EXPECT_EQ(dsu_3.find(i), i);
        EXPECT_EQ(dsu_3.size(i), 1);
        EXPECT_FALSE(dsu_3.connected(i, (i+1)%3)); 
        EXPECT_TRUE(dsu_3.contains(i));
    }
}
    
TEST_F(FastDSUTest, CompressOperation) {
    FastDSU dsu_3(3);
    dsu_3.unionSets(0,1);
    dsu_3.unionSets(1,2); 
    
    dsu_3.compress(); 
    int root = dsu_3.find(2);
    EXPECT_EQ(dsu_3.find(0), root); 
    EXPECT_EQ(dsu_3.find(1), root); 
    EXPECT_TRUE(dsu_3.connected(0,2));
    EXPECT_EQ(dsu_3.size(0), 3);
}

TEST_F(FastDSUTest, IsEmptyAdvanced) {
    FastDSU dsu_0(0);
    EXPECT_TRUE(dsu_0.isEmpty());

    FastDSU dsu_5(5);
    EXPECT_FALSE(dsu_5.isEmpty());
    dsu_5.reset(); 
    EXPECT_FALSE(dsu_5.isEmpty()); 
}

TEST_F(FastDSUTest, ContainsAdvanced) {
    FastDSU dsu_3(3);
    EXPECT_TRUE(dsu_3.contains(0));
    EXPECT_TRUE(dsu_3.contains(1));
    EXPECT_TRUE(dsu_3.contains(2));
    EXPECT_FALSE(dsu_3.contains(3));   
    EXPECT_FALSE(dsu_3.contains(-1));  

    FastDSU dsu_0(0);
    EXPECT_FALSE(dsu_0.contains(0)); 
}

TEST_F(FastDSUTest, PathCompressionChain) {
    FastDSU dsu_5(5); 
    dsu_5.unionSets(0,1);
    dsu_5.unionSets(1,2);
    dsu_5.unionSets(2,3);
    dsu_5.unionSets(3,4); 

    int root = dsu_5.find(4); 
    
    EXPECT_EQ(dsu_5.find(0), root); 
    EXPECT_EQ(dsu_5.find(1), root);
    EXPECT_EQ(dsu_5.find(2), root);
    EXPECT_EQ(dsu_5.find(3), root);
    
    EXPECT_TRUE(dsu_5.connected(0,4));
    EXPECT_EQ(dsu_5.size(0), 5);
    EXPECT_EQ(dsu_5.size(4), 5);
}

TEST_F(FastDSUTest, BoundaryConditions) {
    FastDSU dsu_10(10); 
    EXPECT_TRUE(dsu_10.unionSets(0, 9)); 
    EXPECT_EQ(dsu_10.countSets(), 9);
    EXPECT_TRUE(dsu_10.connected(0,9));
    EXPECT_EQ(dsu_10.size(0), 2);
    EXPECT_EQ(dsu_10.size(9), 2);

    EXPECT_TRUE(dsu_10.unionSets(5,0)); 
    EXPECT_EQ(dsu_10.countSets(), 8);
    EXPECT_TRUE(dsu_10.connected(5,9)); 
    EXPECT_EQ(dsu_10.size(0), 3); 
    EXPECT_EQ(dsu_10.size(5), 3);
    EXPECT_EQ(dsu_10.size(9), 3);

    FastDSU dsu_2(2); 
    EXPECT_TRUE(dsu_2.unionSets(1,0));
    EXPECT_EQ(dsu_2.countSets(),1);
    EXPECT_EQ(dsu_2.size(0),2);
}

// =====================================================================================
// Performance Tests (General, not part of a fixture)
// =====================================================================================

TEST(DSUPerformance, GenericDSULargeScaleOperations) {
    DisjointSetUnion<int> dsu;
    const int num_elements = 100000; 
    const int num_unions = 50000;

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_elements; ++i) {
        dsu.makeSet(i);
    }

    for (int i = 0; i < num_unions; ++i) {
        dsu.unionSets(i % num_elements, (i * 13 + num_unions / 4) % num_elements);
    }
    
    for (int i = 0; i < num_elements; ++i) {
        dsu.find(i % num_elements);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "[ INFO     ] GenericDSU " << num_elements << " elements, "
              << num_unions << " unions, " << num_elements << " finds took: "
              << duration.count() << " ms." << std::endl;
    SUCCEED(); 
}

TEST(DSUPerformance, FastDSULargeScaleOperations) {
    const int num_elements = 100000;
    const int num_unions = 50000;
    FastDSU dsu(num_elements); 

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_unions; ++i) {
        dsu.unionSets(i % num_elements, (i * 13 + num_unions / 4) % num_elements);
    }
    
    for (int i = 0; i < num_elements; ++i) {
        dsu.find(i % num_elements);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "[ INFO     ] FastDSU " << num_elements << " elements, "
              << num_unions << " unions, " << num_elements << " finds took: "
              << duration.count() << " ms." << std::endl;
    SUCCEED();
}
