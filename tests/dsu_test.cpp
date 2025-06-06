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

// Custom Hasher and KeyEqual for CustomData
struct CustomDataHasher {
    size_t operator()(const CustomData& cd) const {
        // A different hash logic than std::hash<CustomData>
        size_t h1 = std::hash<int>()(cd.id);
        size_t h2 = std::hash<std::string>()(cd.name);
        return (h1 * 31) ^ (h2 * 17); // Example different logic
    }
};

struct CustomDataKeyEqual {
    bool operator()(const CustomData& lhs, const CustomData& rhs) const {
        // Could be the same as operator==, but defined separately
        return lhs.id == rhs.id && lhs.name == rhs.name;
    }
};

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

TYPED_TEST(DSTest, UnionSetsBySizeLogic) {
    DisjointSetUnion<TypeParam> dsu(UnionStrategy::BY_SIZE);
    TypeParam v1 = DSTest<TypeParam>::CreateVal(1, "BS_V1");
    TypeParam v2 = DSTest<TypeParam>::CreateVal(2, "BS_V2");
    TypeParam v3 = DSTest<TypeParam>::CreateVal(3, "BS_V3");

    // Test case 1: Union of sets with different sizes
    dsu.makeSet(v1); dsu.makeSet(v2); dsu.makeSet(v3);
    // Initial ranks (assuming default DSU behavior for makeSet)
    // For DisjointSetUnion<T>, rank is stored in a map, access needs find.
    // We can't directly access rank easily here without find and knowing the root.
    // We'll focus on size and parent relationships.

    dsu.unionSets(v1, v2); // Set {v1,v2} size 2; v3 size 1
    EXPECT_EQ(dsu.size(v1), 2);
    EXPECT_EQ(dsu.size(v3), 1);
    TypeParam root_v1_old_tc1 = dsu.find(v1); // Root of {v1,v2}
    TypeParam root_v3_old_tc1 = dsu.find(v3); // Root of {v3}

    EXPECT_TRUE(dsu.unionSets(v1, v3)); // Unioning {v1,v2} (size 2) with {v3} (size 1)
    // Root of smaller set (v3's old root) should attach to root of larger set (v1's old root)
    EXPECT_EQ(dsu.find(v3), root_v1_old_tc1);
    EXPECT_EQ(dsu.find(v1), root_v1_old_tc1);
    EXPECT_EQ(dsu.size(v1), 3);
    EXPECT_EQ(dsu.size(v3), 3);
    EXPECT_TRUE(dsu.connected(v1, v3));

    // Test case 2: Union of sets with equal sizes
    DisjointSetUnion<TypeParam> dsu_eq(UnionStrategy::BY_SIZE);
    TypeParam v4 = DSTest<TypeParam>::CreateVal(4, "BS_V4");
    TypeParam v5 = DSTest<TypeParam>::CreateVal(5, "BS_V5");
    TypeParam v6 = DSTest<TypeParam>::CreateVal(6, "BS_V6");
    TypeParam v7 = DSTest<TypeParam>::CreateVal(7, "BS_V7");

    dsu_eq.makeSet(v4); dsu_eq.makeSet(v5); dsu_eq.makeSet(v6); dsu_eq.makeSet(v7);

    dsu_eq.unionSets(v4, v5); // Set {v4,v5} size 2
    EXPECT_EQ(dsu_eq.size(v4), 2);
    dsu_eq.unionSets(v6, v7); // Set {v6,v7} size 2
    EXPECT_EQ(dsu_eq.size(v6), 2);

    TypeParam root_v4_old_tc2 = dsu_eq.find(v4);
    TypeParam root_v6_old_tc2 = dsu_eq.find(v6);
    EXPECT_NE(root_v4_old_tc2, root_v6_old_tc2);

    EXPECT_TRUE(dsu_eq.unionSets(v4, v6)); // Unioning two sets of size 2. v4 is x, v6 is y.
                                         // Root of x (v4's old root) should attach to root of y (v6's old root).
    EXPECT_EQ(dsu_eq.find(v4), root_v6_old_tc2);
    EXPECT_EQ(dsu_eq.find(v6), root_v6_old_tc2);
    EXPECT_EQ(dsu_eq.size(v4), 4);
    EXPECT_EQ(dsu_eq.size(v6), 4);
    EXPECT_TRUE(dsu_eq.connected(v4, v6));

    // Optional: Rank check (difficult for generic DSU without direct rank access or specific root knowledge)
    // The logic is that rank should not change for BY_SIZE strategy as it does for BY_RANK.
    // For example, if both old roots had rank 0, the new root's rank should still be 0.
    // This is implicitly tested by the union logic adhering to size rather than rank.
}

TYPED_TEST(DSTest, PathCompressionExplicit) {
    DisjointSetUnion<TypeParam> dsu; // Default strategy (BY_RANK) is fine for this test
    TypeParam v0 = DSTest<TypeParam>::CreateVal(0, "PCV0");
    TypeParam v1 = DSTest<TypeParam>::CreateVal(1, "PCV1");
    TypeParam v2 = DSTest<TypeParam>::CreateVal(2, "PCV2");
    TypeParam v3 = DSTest<TypeParam>::CreateVal(3, "PCV3");

    dsu.makeSet(v0); dsu.makeSet(v1); dsu.makeSet(v2); dsu.makeSet(v3);

    // Create a chain: v0 -> v1 -> v2 -> v3 (v3 is the root)
    // unionSets(x,y) makes root of y parent of root of x if ranks are equal and x's rank doesn't increase.
    // Or if rank[rootX] < rank[rootY], parent[rootX] = rootY.
    // To build a chain v0->v1->v2->v3, we want:
    // parent[v0]=v1, parent[v1]=v2, parent[v2]=v3.
    // This happens if v1 becomes root of v0, v2 root of v1, v3 root of v2.
    // With union by rank, and all ranks initially 0:
    // unionSets(v0,v1): parent[v0]=v1, rank[v1]=1
    // unionSets(v1,v2): parent[v1]=v2, rank[v2]=1 (assuming v1's rank was 1, v2's was 0. So v2 becomes parent)
    // This doesn't quite work as expected. Let's re-evaluate chain creation.
    // If we do:
    // dsu.unionSets(v0, v1); // parent[v0]=v1, rank[v1]=1 (v0 is child of v1)
    // dsu.unionSets(v2, v3); // parent[v2]=v3, rank[v3]=1 (v2 is child of v3)
    // dsu.unionSets(v1, v3); // parent[v1]=v3 (v1 child of v3, as rank[v1]=1, rank[v3]=1, v3 becomes parent, rank[v3]=2)
    // Now we have v0 -> v1 -> v3 and v2 -> v3.
    // This is a decent structure. find(v0) should make parent[v0]=v3.

    // Let's use the structure: v0 -> v1, v1 -> v2, v2 -> v3
    // This structure is typically formed if v1's rank is less than v2's, and v2's less than v3's,
    // or by specific tie-breaking.
    // With default union-by-rank and initial ranks 0:
    // 1. unionSets(v0, v1) => parent[v0]=v1, rank[v1]=1
    // 2. unionSets(v1, v2) => Here, find(v1) is v1, find(v2) is v2. rank[v1]=1, rank[v2]=0. So parent[v2]=v1.
    //    This creates v2 -> v1, v0 -> v1. Not a chain v0->v1->v2.

    // To reliably create a chain for testing path compression, especially with union-by-rank,
    // it's often easier to make elements children of a single root one by one, then find the deepest.
    // Or, if we control ranks, we can make a chain.
    // Given we use unionSets, which itself uses find(), some compression might happen during setup.
    // The test below will assume unionSets(child, parent_candidate) attempts to make child point to parent_candidate's root.

    // Setup: v0, v1, v2 are leaves, v3 is their common root, but v0->v1, v1->v2, v2->v3 (conceptual chain before first full find)
    // This exact parent chain is hard to guarantee with union by rank/size without direct parent setting.
    // Let's re-think the chain setup for generic DSU.
    // Create a structure: v0, v1, v2, v3.
    // Union v0 and v1. Let's say v1 becomes root of v0. (parent[v0] = v1)
    dsu.unionSets(v0, v1);
    // Union v1 and v2. If v2 becomes root of v1. (parent[v1] = v2)
    // To encourage v2 to be root of v1:
    // If using union by rank, after (v0,v1), rank[v1] is 1. rank[v2] is 0.
    // So unionSets(v1,v2) -> find(v1)=v1, find(v2)=v2. rank[v1]=1, rank[v2]=0. parent[v2]=v1.
    // This makes v0->v1, v2->v1. Not a chain.

    // A better way to test path compression:
    // 1. Create elements v0, v1, v2, v3.
    // 2. Manually set parents if possible (not here).
    // 3. Or, create a deep structure:
    //    dsu.unionSets(v0,v1); // v0 -> v1 (v1 is root, rank 1)
    //    dsu.unionSets(v2,v0); // v2 -> v1 (v0's root is v1, v2's rank 0, v1's rank 1. v2 becomes child of v1)
    //    dsu.unionSets(v3,v2); // v3 -> v1 (v2's root is v1, v3's rank 0, v1's rank 1. v3 becomes child of v1)
    //    At this point, v0,v2,v3 are direct children of v1. This is not a chain.

    // Let's try a specific sequence with UnionByRank (default for DisjointSetUnion if not specified)
    // Ranks: v0:0, v1:0, v2:0, v3:0
    dsu.unionSets(v0, v1); // parent[v0]=v1, rank[v1]=1. v0 is child of v1.
    // Ranks: v0:0, v1:1, v2:0, v3:0
    // Parents: v0:v1
    // Now, union v1 and v2. find(v1)=v1, find(v2)=v2. rank[v1]=1, rank[v2]=0.
    // So, v2 becomes child of v1. parent[v2]=v1.
    dsu.unionSets(v2, v1); // v2 is child of v1.
    // Ranks: v0:0, v1:1, v2:0, v3:0
    // Parents: v0:v1, v2:v1
    // Now, union v1 and v3. find(v1)=v1, find(v3)=v3. rank[v1]=1, rank[v3]=0.
    // So, v3 becomes child of v1. parent[v3]=v1.
    dsu.unionSets(v3, v1); // v3 is child of v1.
    // All v0,v2,v3 are children of v1. This is a star, not a chain. Path compression on find(v0) changes nothing.

    // To test path compression, we need a path of nodes.
    // v0 -> v1 -> v2 -> v3 ... -> root
    // A sequence of unions like dsu.unionSets(v_i, v_{i+1}) will create this if v_{i+1} always becomes the root.
    // For union-by-rank, this happens if rank[v_i] < rank[v_{i+1}] or if ranks are equal and v_{i+1} is chosen.
    // Let's use a fresh DSU for clarity on ranks.
    DisjointSetUnion<TypeParam> dsu_pc;
    TypeParam e0 = DSTest<TypeParam>::CreateVal(100, "E0");
    TypeParam e1 = DSTest<TypeParam>::CreateVal(101, "E1");
    TypeParam e2 = DSTest<TypeParam>::CreateVal(102, "E2");
    TypeParam e3 = DSTest<TypeParam>::CreateVal(103, "E3");
    dsu_pc.makeSet(e0); dsu_pc.makeSet(e1); dsu_pc.makeSet(e2); dsu_pc.makeSet(e3);

    // Goal: e0 -> e1 -> e2 -> e3 (e3 is root)
    // 1. unionSets(e0, e1): parent[e0]=e1, rank[e1]=1. (Correct)
    // 2. unionSets(e1, e2): find(e1)=e1, find(e2)=e2. rank[e1]=1, rank[e2]=0. So parent[e2]=e1. (Incorrect for chain)
    // To make e1 child of e2: we need rank[e1] <= rank[e2]. If rank[e1]=0, rank[e2]=0, then union(e1,e2) -> parent[e1]=e2, rank[e2]=1.

    // Re-strategize: Use a DSU that is always union by X being child of Y.
    // The current unionSets(x,y) with BY_RANK:
    // if rank[rootX] < rank[rootY] -> parent[rootX] = rootY
    // if rank[rootX] > rank[rootY] -> parent[rootY] = rootX
    // if rank[rootX] == rank[rootY] -> parent[rootX] = rootY, rank[rootY]++
    // So, if we call unionSets(child, parent_candidate), and child's rank is less than or equal to parent_candidate's rank,
    // child's root will point to parent_candidate's root.

    // Build chain e0 -> e1 -> e2 -> e3 (e3 is the overall root)
    // All start with rank 0.
    // 1. dsu_pc.unionSets(e0, e1); // e0 points to e1 (parent[e0]=e1), rank[e1] becomes 1.
    // 2. dsu_pc.unionSets(e1, e2); // rootE1=e1 (rank 1), rootE2=e2 (rank 0). rootE2 points to rootE1. parent[e2]=e1.
                               // This is not e1->e2. It becomes e0->e1, e2->e1.
    // The only way to force a chain with current unionSets is if the "parent" side of union always has higher rank.
    // This is not naturally occurring for a simple chain.

    // The test `PathCompressionChain` already tests the effect of path compression implicitly.
    // This `PathCompressionExplicit` test aims to check direct parent pointers.
    // Let's assume the structure from `PathCompressionChain` test:
    // v1-v2, v2-v3, v3-v4, v4-v5 (using default DSU, which is BY_RANK)
    // dsu_alt.unionSets(v1,v2); parent[v1]=v2, rank[v2]=1
    // dsu_alt.unionSets(v2,v3); parent[v2]=v3, rank[v3]=2 (as rank[v2_root=v3]=1, rank[v3_root=v3]=0 initially. this is wrong)
    // Let's trace carefully:
    DisjointSetUnion<TypeParam> dsu_alt;
    TypeParam pc_v0 = DSTest<TypeParam>::CreateVal(200, "PCV0_alt");
    TypeParam pc_v1 = DSTest<TypeParam>::CreateVal(201, "PCV1_alt");
    TypeParam pc_v2 = DSTest<TypeParam>::CreateVal(202, "PCV2_alt");
    TypeParam pc_v3 = DSTest<TypeParam>::CreateVal(203, "PCV3_alt");

    dsu_alt.makeSet(pc_v0); dsu_alt.makeSet(pc_v1); dsu_alt.makeSet(pc_v2); dsu_alt.makeSet(pc_v3);
    // Chain: pc_v0 -> pc_v1 -> pc_v2 -> pc_v3 (pc_v3 is root)
    // Union (child, parent_candidate)
    // 1. union(pc_v0, pc_v1): parent[pc_v0]=pc_v1, rank[pc_v1]=1.
    dsu_alt.unionSets(pc_v0, pc_v1);
    // Current state: pc_v0 -> pc_v1. Direct parent of pc_v0 is pc_v1.
    ASSERT_EQ(dsu_alt.getDirectParent_Test(pc_v0), pc_v1);

    // 2. union(pc_v1, pc_v2): root of pc_v1 is pc_v1 (rank 1). root of pc_v2 is pc_v2 (rank 0).
    //    Result: parent[pc_v2]=pc_v1. This is not the chain we want.
    // We want parent[pc_v1]=pc_v2. This requires rank[pc_v1] <= rank[pc_v2].
    // To achieve pc_v0 -> pc_v1 -> pc_v2:
    // Make pc_v2 have rank 1 (e.g. pc_v2_dummy -> pc_v2)
    // Then union(pc_v1, pc_v2) where rank[pc_v1]=0, rank[pc_v2]=1 -> parent[pc_v1]=pc_v2

    // Let's use a simpler setup where path compression is unambiguous.
    // Create elements: root, child1, child2, grandchild1 (of child1)
    // grandchild1 -> child1 -> root
    // child2 -> root
    TypeParam root_val = DSTest<TypeParam>::CreateVal(300, "Root");
    TypeParam child1_val = DSTest<TypeParam>::CreateVal(301, "Child1");
    TypeParam child2_val = DSTest<TypeParam>::CreateVal(302, "Child2");
    TypeParam grandchild1_val = DSTest<TypeParam>::CreateVal(303, "GrandChild1");

    DisjointSetUnion<TypeParam> dsu_explicit;
    dsu_explicit.makeSet(root_val);
    dsu_explicit.makeSet(child1_val);
    dsu_explicit.makeSet(child2_val);
    dsu_explicit.makeSet(grandchild1_val);

    // Build structure: grandchild1 -> child1, child1 -> root_val, child2 -> root_val
    // 1. Union grandchild1 and child1. Let child1 be root of grandchild1.
    dsu_explicit.unionSets(grandchild1_val, child1_val); // parent[grandchild1]=child1, rank[child1]=1
    ASSERT_EQ(dsu_explicit.getDirectParent_Test(grandchild1_val), child1_val);

    // 2. Union child1 and root_val. Let root_val be root of child1.
    //    find(child1)=child1 (rank 1), find(root_val)=root_val (rank 0)
    //    Result: parent[root_val]=child1. This is not child1 -> root_val.
    // To make child1 child of root_val, we need rank[child1] <= rank[root_val].
    // If we union (child1, root_val): parent[child1]=root_val IF rank[child1] <= rank[root_val].
    // Let's make rank of root_val higher first.
    TypeParam dummy_for_rank = DSTest<TypeParam>::CreateVal(999, "Dummy");
    dsu_explicit.makeSet(dummy_for_rank);
    dsu_explicit.unionSets(dummy_for_rank, root_val); // parent[dummy]=root_val, rank[root_val]=1.

    // Now, rank[child1]=1, rank[root_val]=1.
    // unionSets(child1, root_val): parent[child1]=root_val, rank[root_val]=2.
    dsu_explicit.unionSets(child1_val, root_val);
    ASSERT_EQ(dsu_explicit.getDirectParent_Test(child1_val), root_val);
    // Path is: grandchild1_val -> child1_val -> root_val

    // 3. Union child2 and root_val. Let root_val be root of child2.
    //    find(child2)=child2 (rank 0), find(root_val)=root_val (rank 2).
    //    Result: parent[child2]=root_val.
    dsu_explicit.unionSets(child2_val, root_val);
    ASSERT_EQ(dsu_explicit.getDirectParent_Test(child2_val), root_val);

    // Verify initial direct parents in the chain grandchild1_val -> child1_val -> root_val
    ASSERT_EQ(dsu_explicit.getDirectParent_Test(grandchild1_val), child1_val);
    ASSERT_EQ(dsu_explicit.getDirectParent_Test(child1_val), root_val);
    ASSERT_EQ(dsu_explicit.getDirectParent_Test(root_val), root_val); // Root points to itself

    // Call find on grandchild1_val to trigger path compression
    TypeParam ultimate_root = dsu_explicit.find(grandchild1_val);
    EXPECT_EQ(ultimate_root, root_val);

    // After path compression:
    // grandchild1_val should point directly to root_val.
    // child1_val (intermediate) should also point directly to root_val.
    EXPECT_EQ(dsu_explicit.getDirectParent_Test(grandchild1_val), root_val);
    EXPECT_EQ(dsu_explicit.getDirectParent_Test(child1_val), root_val);
    // child2's parent should remain root_val, unaffected by find(grandchild1_val).
    EXPECT_EQ(dsu_explicit.getDirectParent_Test(child2_val), root_val);
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

TEST(DSUCustomFunctorsTest, OperationsWithCustomFunctors) {
    DisjointSetUnion<CustomData, CustomDataHasher, CustomDataKeyEqual> dsu_custom_functors;

    CustomData cd1 = {10, "TestAlice"};
    CustomData cd2 = {20, "TestBob"};
    CustomData cd3 = {30, "TestCharlie"};

    dsu_custom_functors.makeSet(cd1);
    dsu_custom_functors.makeSet(cd2);
    dsu_custom_functors.makeSet(cd3);

    EXPECT_EQ(dsu_custom_functors.countSets(), 3);
    EXPECT_EQ(dsu_custom_functors.size(cd1), 1);
    EXPECT_EQ(dsu_custom_functors.size(cd2), 1);
    EXPECT_EQ(dsu_custom_functors.size(cd3), 1);
    EXPECT_TRUE(dsu_custom_functors.contains(cd1));
    EXPECT_FALSE(dsu_custom_functors.connected(cd1, cd2));

    EXPECT_TRUE(dsu_custom_functors.unionSets(cd1, cd2));
    EXPECT_TRUE(dsu_custom_functors.connected(cd1, cd2));
    EXPECT_FALSE(dsu_custom_functors.connected(cd1, cd3));
    EXPECT_EQ(dsu_custom_functors.size(cd1), 2);
    EXPECT_EQ(dsu_custom_functors.size(cd2), 2);
    EXPECT_EQ(dsu_custom_functors.size(cd3), 1);
    EXPECT_EQ(dsu_custom_functors.countSets(), 2);

    EXPECT_TRUE(dsu_custom_functors.unionSets(cd2, cd3));
    EXPECT_TRUE(dsu_custom_functors.connected(cd1, cd3));
    EXPECT_TRUE(dsu_custom_functors.connected(cd2, cd3));
    EXPECT_EQ(dsu_custom_functors.size(cd1), 3);
    EXPECT_EQ(dsu_custom_functors.size(cd2), 3);
    EXPECT_EQ(dsu_custom_functors.size(cd3), 3);
    EXPECT_EQ(dsu_custom_functors.countSets(), 1);

    CustomData cd_non_existent = {99, "NonExistent"};
    EXPECT_FALSE(dsu_custom_functors.contains(cd_non_existent));
    EXPECT_EQ(dsu_custom_functors.find(cd_non_existent), cd_non_existent); // Auto-creates
    EXPECT_TRUE(dsu_custom_functors.contains(cd_non_existent));
    EXPECT_EQ(dsu_custom_functors.countSets(), 2); // One new set for cd_non_existent
    EXPECT_EQ(dsu_custom_functors.size(cd_non_existent), 1);
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

TEST_F(FastDSUTest, UnionSetsBySizeLogic) {
    FastDSU dsu(10, UnionStrategy::BY_SIZE);

    // Test case 1: Union of sets with different sizes
    // Elements 0, 1, 2. Initially ranks are 0, sizes are 1.
    // initial ranks: rank[0]=0, rank[1]=0, rank[2]=0
    // initial sizes: setSize[0]=1, setSize[1]=1, setSize[2]=1

    dsu.unionSets(0, 1); // Set {0,1} size 2. Element 2 is size 1.
                         // New root for {0,1} could be 0 or 1. Let's assume 1 (as per current by_rank tie-breaking if sizes are 1 and ranks 0)
                         // If 1 becomes root of 0: parent[0]=1, setSize[1]=2. rank[1] might become 1 if it was by_rank.
                         // For BY_SIZE, if initial sizes are 1, it's an equal size union.
                         // If 0 is x, 1 is y: parent[0]=1, setSize[1]+=setSize[0] -> setSize[1]=2. Rank of 1 is not incremented.
    EXPECT_EQ(dsu.size(0), 2);
    EXPECT_EQ(dsu.size(1), 2);
    EXPECT_EQ(dsu.size(2), 1);

    int root0_old_tc1 = dsu.find(0); // This is the root of {0,1}
    int root2_old_tc1 = dsu.find(2); // This is 2 itself
    EXPECT_NE(root0_old_tc1, root2_old_tc1);

    // Store ranks before union for verification (requires rank access)
    // NOTE: This part of the test assumes 'rank' member of FastDSU is accessible.
    // If not, these specific rank assertions might need to be removed or adapted.
    // std::vector<int> ranks_before_tc1 = dsu.rank; // Hypothetical direct access

    EXPECT_TRUE(dsu.unionSets(0, 2)); // Unioning set {0,1} (size 2, root root0_old_tc1) with set {2} (size 1, root root2_old_tc1)
                                      // Root of smaller set {2} (root2_old_tc1) should attach to root of larger set {0,1} (root0_old_tc1).
    EXPECT_EQ(dsu.find(2), root0_old_tc1);
    EXPECT_EQ(dsu.find(0), root0_old_tc1);
    EXPECT_EQ(dsu.size(0), 3);
    EXPECT_EQ(dsu.size(2), 3);
    EXPECT_TRUE(dsu.connected(0, 2));

    // Verify ranks didn't change like in union-by-rank
    // e.g. rank[root0_old_tc1] should not have incremented due to this union by size.
    // EXPECT_EQ(dsu.rank[root0_old_tc1], ranks_before_tc1[root0_old_tc1]); // Assuming rank access

    // Test case 2: Union of sets with equal sizes
    // Elements 3,4,5,6. ranks are 0, sizes are 1 initially.
    // dsu state: {0,1,2}, {3}, {4}, {5}, {6}, ...
    // rank[3]=0, size[3]=1. rank[4]=0, size[4]=1 etc.

    dsu.unionSets(3, 4); // Set {3,4} size 2. Assume 4 is root of 3. parent[3]=4, setSize[4]=2. rank[4] unchanged (0).
    EXPECT_EQ(dsu.size(3), 2);
    dsu.unionSets(5, 6); // Set {5,6} size 2. Assume 6 is root of 5. parent[5]=6, setSize[6]=2. rank[6] unchanged (0).
    EXPECT_EQ(dsu.size(5), 2);

    int root3_old_tc2 = dsu.find(3); // root of {3,4}, e.g. 4
    int root5_old_tc2 = dsu.find(5); // root of {5,6}, e.g. 6
    EXPECT_NE(root3_old_tc2, root5_old_tc2);

    // Store ranks before union for verification (requires rank access)
    // std::vector<int> ranks_before_tc2 = dsu.rank; // Hypothetical direct access
    // int rank_root3_old_val = ranks_before_tc2[root3_old_tc2];
    // int rank_root5_old_val = ranks_before_tc2[root5_old_tc2];


    EXPECT_TRUE(dsu.unionSets(3, 5)); // Unioning {3,4} (size 2) with {5,6} (size 2).
                                      // 3 is x, 5 is y. Root of x (root3_old_tc2) should attach to root of y (root5_old_tc2).
    EXPECT_EQ(dsu.find(3), root5_old_tc2);
    EXPECT_EQ(dsu.find(5), root5_old_tc2);
    EXPECT_EQ(dsu.find(root3_old_tc2), root5_old_tc2); // old root of x is now child of old root of y
    EXPECT_EQ(dsu.size(3), 4);
    EXPECT_EQ(dsu.size(5), 4);
    EXPECT_TRUE(dsu.connected(3, 5));

    // Verify rank of the new root (root5_old_tc2) did not increment due to this equal size union by size.
    // It should remain what it was (e.g. 0 if these were its first unions).
    // EXPECT_EQ(dsu.rank[root5_old_tc2], rank_root5_old_val); // Assuming rank access
    // If rank_root3_old_val was also 0, and rank_root5_old_val was 0,
    // then for Union By Rank, rank[root5_old_tc2] would become 1. Here it should not.
}

TEST_F(FastDSUTest, PathCompressionExplicit) {
    FastDSU dsu(4); // Elements 0, 1, 2, 3. Default strategy BY_RANK.

    // Goal: Create a chain 0 -> 1 -> 2 -> 3 (3 is the root)
    // This requires careful calls to unionSets if using Union By Rank.
    // unionSets(x,y): if ranks are equal, x's root becomes child of y's root, y's rank increments.
    // To make 0 child of 1: unionSets(0,1) -> parent[0]=1, rank[1]=1
    // To make 1 child of 2: unionSets(1,2) -> (rank[1]=1, rank[2]=0) -> parent[2]=1. Not what we want.
    // We need rank[1] <= rank[2].
    // For FastDSU, ranks are directly manipulated by union by rank.
    // Let's use a fresh DSU instance for each step if needed, or be very careful.

    // Simpler setup for FastDSU: 0->1, 1->2. Then find(0).
    // 1. unionSets(0,1): parent[0]=1, rank[1]=1.
    dsu.unionSets(0,1);
    ASSERT_EQ(dsu.getDirectParent_Test(0), 1);
    ASSERT_EQ(dsu.getDirectParent_Test(1), 1); // 1 is root

    // 2. To make 1 child of 2 (i.e. 0->1->2), we need parent[1]=2.
    //    Currently rank[1]=1, rank[2]=0.
    //    unionSets(1,2): find(1)=1, find(2)=2. rank[1]=1 > rank[2]=0. So parent[2]=1.
    //    Structure: 0->1, 2->1. This is not a chain 0->1->2.

    // Let's use the same structure as the generic DSU test for consistency:
    // grandchild (0) -> child1 (1) -> root (2)
    // child2 (3) -> root (2)
    FastDSU dsu_explicit(4); // 0,1,2,3

    // 1. grandchild (0) -> child1 (1)
    //    unionSets(0,1): parent[0]=1, rank[1]=1
    dsu_explicit.unionSets(0,1);
    ASSERT_EQ(dsu_explicit.getDirectParent_Test(0), 1);

    // 2. child1 (1) -> root (2)
    //    Need rank[1] <= rank[2]. Currently rank[1]=1, rank[2]=0.
    //    To make parent[1]=2, we need root(1) to become child of root(2).
    //    So, unionSets(1,2) will make parent[2]=1.
    //    Let's make rank[2] higher first.
    //    FastDSU dsu_rank_setup(10); // use a different DSU to setup ranks if needed or use dummy unions
    //    dsu_explicit.unionSets(element_to_sacrifice, 2); // make rank[2]=1
    //    This is getting complicated. The key is to have an uncompressed path.
    //    If we set parent[0]=1, parent[1]=2, parent[2]=2 (root).
    //    This can be achieved if:
    //    union(0,1) -> p[0]=1, r[1]=1
    //    union(other,2) -> p[other]=2, r[2]=1 (to make r[2] same as r[1])
    //    union(1,2) -> p[1]=2, r[2]=2
    FastDSU dsu_chain(4); // 0,1,2,3. Element 3 is dummy for rank.
    dsu_chain.unionSets(0,1); // p[0]=1, r[1]=1
    dsu_chain.unionSets(3,2); // p[3]=2, r[2]=1 (dummy union to raise rank of 2)

    // Now, r[find(1)]=r[1]=1. r[find(2)]=r[2]=1.
    dsu_chain.unionSets(1,2); // p[1]=2, r[2]=2. (Chain: 0->1->2)

    ASSERT_EQ(dsu_chain.getDirectParent_Test(0), 1);
    ASSERT_EQ(dsu_chain.getDirectParent_Test(1), 2);
    ASSERT_EQ(dsu_chain.getDirectParent_Test(2), 2); // 2 is root

    // Call find on 0 to trigger path compression for 0 and 1.
    int final_root = dsu_chain.find(0);
    EXPECT_EQ(final_root, 2);

    // After path compression:
    // 0 should point directly to 2.
    // 1 (intermediate) should also point directly to 2.
    EXPECT_EQ(dsu_chain.getDirectParent_Test(0), 2);
    EXPECT_EQ(dsu_chain.getDirectParent_Test(1), 2);
    // Element 3 is still child of 2 (or 2 is child of 3 if tiebreak went other way, but find(0) shouldn't affect it)
    // Let's check find(3) to be sure of its root if we care.
    // For this test, we only care about the path from 0.
    ASSERT_EQ(dsu_chain.getDirectParent_Test(3), 2); // Parent of 3 is 2 from unionSets(3,2) where r[2] became 1
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
