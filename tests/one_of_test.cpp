#include "gtest/gtest.h"  // Google Test header
#include "one_of.h"       // Path to one_of.h, assuming CMake handles include paths

#include <string>
#include <vector>
#include <typeindex>
#include <stdexcept> // For std::bad_variant_access

// Helper structs/classes for testing
struct TestTypeA {
    int id;
    static int constructions;
    static int destructions;
    static int copies;
    static int moves;

    TestTypeA(int i = 0) : id(i) { constructions++; }
    ~TestTypeA() { destructions++; }
    TestTypeA(const TestTypeA& other) : id(other.id) { copies++; constructions++; }
    TestTypeA(TestTypeA&& other) noexcept : id(other.id) { other.id = -1; moves++; constructions++; }

    TestTypeA& operator=(const TestTypeA& other) {
        if (this != &other) {
            id = other.id;
            copies++;
        }
        return *this;
    }
    TestTypeA& operator=(TestTypeA&& other) noexcept {
        if (this != &other) {
            id = other.id;
            other.id = -1;
            moves++;
        }
        return *this;
    }
    bool operator==(const TestTypeA& other) const { return id == other.id; }
    static void reset_counts() { constructions = 0; destructions = 0; copies = 0; moves = 0; }
};
int TestTypeA::constructions = 0;
int TestTypeA::destructions = 0;
int TestTypeA::copies = 0;
int TestTypeA::moves = 0;

struct TestTypeB {
    std::string data;
    static int constructions;
    static int destructions;
    static int copies; // Added for consistency if we track copies
    static int moves;  // Added for consistency if we track moves

    TestTypeB(std::string s = "") : data(std::move(s)) { constructions++; }
    ~TestTypeB() { destructions++; }
    TestTypeB(const TestTypeB& other) : data(other.data) { constructions++; copies++; }
    TestTypeB(TestTypeB&& other) noexcept : data(std::move(other.data)) { constructions++; moves++; }

    TestTypeB& operator=(const TestTypeB& other) {
        if (this != &other) {
            data = other.data;
            copies++;
        }
        return *this;
    }
    TestTypeB& operator=(TestTypeB&& other) noexcept {
        if (this != &other) {
            data = std::move(other.data);
            moves++;
        }
        return *this;
    }
    bool operator==(const TestTypeB& other) const { return data == other.data; }
    static void reset_counts() { constructions = 0; destructions = 0; copies = 0; moves = 0; }
};
int TestTypeB::constructions = 0;
int TestTypeB::destructions = 0;
int TestTypeB::copies = 0;
int TestTypeB::moves = 0;

struct TestVisitor {
    mutable int visited_a = 0;
    mutable int visited_b = 0;
    mutable int visited_int = 0;
    mutable std::string last_b_val;
    mutable int last_a_id;

    void operator()(const TestTypeA& a) const { visited_a++; last_a_id = a.id; }
    void operator()(TestTypeA& a) { visited_a++; last_a_id = a.id; a.id += 1000; }
    void operator()(const TestTypeB& b) const { visited_b++; last_b_val = b.data; }
    void operator()(TestTypeB& b) { visited_b++; last_b_val = b.data; b.data += " visited"; }
    void operator()(int i) const { visited_int++; (void)i; }
    void operator()(int& i) { visited_int++; i *= 10; }
};

struct MoveOnlyType {
    int id;
    static int constructions;
    static int destructions;
    static int moves;
    MoveOnlyType(int i = 0) : id(i) { constructions++; }
    ~MoveOnlyType() { destructions++; }
    MoveOnlyType(const MoveOnlyType&) = delete;
    MoveOnlyType& operator=(const MoveOnlyType&) = delete;
    MoveOnlyType(MoveOnlyType&& other) noexcept : id(other.id) { other.id = -1; moves++; constructions++; } // construction still happens
    MoveOnlyType& operator=(MoveOnlyType&& other) noexcept {
        if (this != &other) { id = other.id; other.id = -1; moves++; }
        return *this;
    }
    static void reset_counts() { constructions = 0; destructions = 0; moves = 0; }
};
int MoveOnlyType::constructions = 0;
int MoveOnlyType::destructions = 0;
int MoveOnlyType::moves = 0;

class OneOfTest : public ::testing::Test {
protected:
    void SetUp() override {
        TestTypeA::reset_counts();
        TestTypeB::reset_counts();
        MoveOnlyType::reset_counts();
    }
};

TEST_F(OneOfTest, DefaultConstruction) {
    OneOf<TestTypeA, TestTypeB, int> oo;
    ASSERT_FALSE(oo.has_value());
    ASSERT_EQ(oo.index(), std::variant_npos);
    ASSERT_EQ(TestTypeA::constructions, 0);
    ASSERT_EQ(TestTypeB::constructions, 0);
}

TEST_F(OneOfTest, ValueConstructionAndIntrospection) {
    {
        OneOf<TestTypeA, TestTypeB, int> oo(TestTypeA(10));
        ASSERT_TRUE(oo.has_value());
        ASSERT_TRUE(oo.has<TestTypeA>());
        ASSERT_FALSE(oo.has<TestTypeB>());
        ASSERT_FALSE(oo.has<int>());
        ASSERT_EQ(oo.index(), 0);
        ASSERT_EQ(oo.type(), std::type_index(typeid(TestTypeA)));
        ASSERT_NE(oo.get_if<TestTypeA>(), nullptr);
        ASSERT_EQ(oo.get_if<TestTypeA>()->id, 10);
        ASSERT_EQ(TestTypeA::constructions, 2); // Temp + (move) construction in OneOf
        ASSERT_EQ(TestTypeA::moves, 1);
    }
    ASSERT_EQ(TestTypeA::destructions, 2); // Temp + OneOf's content

    TestTypeB::reset_counts();
    {
        OneOf<TestTypeA, TestTypeB, int> oo(TestTypeB("hello"));
        ASSERT_TRUE(oo.has<TestTypeB>());
        ASSERT_EQ(oo.index(), 1);
        ASSERT_EQ(oo.type(), std::type_index(typeid(TestTypeB)));
        ASSERT_EQ(oo.get_if<TestTypeB>()->data, "hello");
        ASSERT_EQ(TestTypeB::constructions, 2); // Temp + (move) construction
        ASSERT_EQ(TestTypeB::moves, 1);
    }
    ASSERT_EQ(TestTypeB::destructions, 2);

    {
        OneOf<TestTypeA, TestTypeB, int> oo(123);
        ASSERT_TRUE(oo.has<int>());
        ASSERT_EQ(oo.index(), 2);
        ASSERT_EQ(oo.type(), std::type_index(typeid(int)));
        ASSERT_EQ(*oo.get_if<int>(), 123);
    }
}

TEST_F(OneOfTest, SetAndEmplace) {
    OneOf<TestTypeA, TestTypeB, int> oo;
    oo.set(TestTypeA(20)); // Temp A, moved into oo. constructions=2, moves=1
    ASSERT_TRUE(oo.has<TestTypeA>());
    ASSERT_EQ(oo.get_if<TestTypeA>()->id, 20);
    ASSERT_EQ(TestTypeA::constructions, 2);
    ASSERT_EQ(TestTypeA::moves, 1);
    int prev_destructions_a = TestTypeA::destructions; // Should be 1 (for temp)

    oo.set(TestTypeB("world")); // A(20) in oo destructs. Temp B, moved into oo.
    ASSERT_TRUE(oo.has<TestTypeB>());
    ASSERT_EQ(oo.get_if<TestTypeB>()->data, "world");
    ASSERT_EQ(TestTypeA::destructions, prev_destructions_a + 1); // A(20) from oo
    ASSERT_EQ(TestTypeB::constructions, 2); // Temp B + move construction
    ASSERT_EQ(TestTypeB::moves, 1);
    int prev_destructions_b = TestTypeB::destructions; // Should be 1 (for temp B)

    // Set with same type
    oo.set(TestTypeB("new world")); // Temp B("new world"). B("world") in oo is assigned from Temp B.
                                    // Current OneOf::set logic: reset() + new construction
                                    // So, B("world") destructs. Temp B("new world") created, then moved.
    ASSERT_TRUE(oo.has<TestTypeB>());
    ASSERT_EQ(oo.get_if<TestTypeB>()->data, "new world");
    ASSERT_EQ(TestTypeB::destructions, prev_destructions_b + 1 + 1); // old B("world") + temp B("new world")
    ASSERT_EQ(TestTypeB::constructions, 2 + 2); // Prev 2 + new Temp B + move constr
    ASSERT_EQ(TestTypeB::moves, 1 + 1);


    prev_destructions_b = TestTypeB::destructions;
    TestTypeA::reset_counts();
    oo.emplace<TestTypeA>(30); // B("new world") destructs. A(30) emplaced.
    ASSERT_TRUE(oo.has<TestTypeA>());
    ASSERT_EQ(oo.get_if<TestTypeA>()->id, 30);
    ASSERT_EQ(TestTypeB::destructions, prev_destructions_b + 1); // B("new world")
    ASSERT_EQ(TestTypeA::constructions, 1); // Emplaced directly
    ASSERT_EQ(TestTypeA::moves, 0);
    ASSERT_EQ(TestTypeA::copies, 0);
}

TEST_F(OneOfTest, Reset) {
    OneOf<TestTypeA, TestTypeB, int> oo(TestTypeA(40)); // Temp A, move to oo. constr=2, moves=1, destr=1 (temp)
    ASSERT_TRUE(oo.has_value());
    ASSERT_EQ(TestTypeA::constructions, 2);
    oo.reset(); // A in oo destructs
    ASSERT_FALSE(oo.has_value());
    ASSERT_EQ(oo.index(), std::variant_npos);
    ASSERT_EQ(TestTypeA::destructions, 1 + 1); // temp + oo's content

    oo.reset(); // No-op
    ASSERT_FALSE(oo.has_value());
    ASSERT_EQ(TestTypeA::destructions, 2);
}

TEST_F(OneOfTest, Visiting) {
    OneOf<TestTypeA, TestTypeB, int> oo;
    TestVisitor visitor;
    oo.emplace<TestTypeA>(50);
    oo.visit(visitor);
    ASSERT_EQ(visitor.visited_a, 1);
    ASSERT_EQ(visitor.visited_b, 0);
    ASSERT_EQ(visitor.visited_int, 0);
    ASSERT_EQ(visitor.last_a_id, 50);
    ASSERT_EQ(oo.get_if<TestTypeA>()->id, 50 + 1000);

    visitor = TestVisitor();
    oo.emplace<TestTypeB>("visit_test"); // A(50+1000) destructs. B emplaced.
    const auto& const_oo = oo;
    const_oo.visit(visitor);
    ASSERT_EQ(visitor.visited_a, 0);
    ASSERT_EQ(visitor.visited_b, 1);
    ASSERT_EQ(visitor.visited_int, 0);
    ASSERT_EQ(visitor.last_b_val, "visit_test");
    ASSERT_EQ(oo.get_if<TestTypeB>()->data, "visit_test"); // No modification from const visit

    visitor = TestVisitor();
    oo.visit(visitor); // Non-const visit on B
    ASSERT_EQ(oo.get_if<TestTypeB>()->data, "visit_test visited");

    visitor = TestVisitor();
    oo.emplace<int>(5); // B("visit_test visited") destructs. int emplaced.
    oo.visit(visitor);
    ASSERT_EQ(visitor.visited_int, 1);
    ASSERT_EQ(*oo.get_if<int>(), 50);

    oo.reset(); // int(50) destructs.
    ASSERT_THROW(oo.visit(visitor), std::bad_variant_access);
}

TEST_F(OneOfTest, TypeMethodException) {
    OneOf<TestTypeA, int> oo;
    ASSERT_THROW(oo.type(), std::bad_variant_access);
}

TEST_F(OneOfTest, CopyConstruction) {
    OneOf<TestTypeA, TestTypeB> oo1(TestTypeA(101)); // Temp A, move to oo1. TTA: constr=2, moves=1, copies=0, destr=1 (temp)
    ASSERT_EQ(TestTypeA::constructions, 2);
    ASSERT_EQ(TestTypeA::moves, 1);
    ASSERT_EQ(TestTypeA::copies, 0);

    OneOf<TestTypeA, TestTypeB> oo2 = oo1; // A in oo1 copied to oo2. TTA: constr=2+1, moves=1, copies=0+1
    ASSERT_TRUE(oo2.has<TestTypeA>());
    ASSERT_EQ(oo2.get_if<TestTypeA>()->id, 101);
    ASSERT_TRUE(oo1.has<TestTypeA>());
    ASSERT_EQ(oo1.get_if<TestTypeA>()->id, 101);

    ASSERT_EQ(TestTypeA::constructions, 3);
    ASSERT_EQ(TestTypeA::copies, 1);
    ASSERT_EQ(TestTypeA::moves, 1);
} // oo1, oo2 destruct. Total TTA destr = 1(temp) + 1(oo1) + 1(oo2) = 3. Matches constr.

TEST_F(OneOfTest, CopyAssignment) {
    OneOf<TestTypeA, TestTypeB> oo1(TestTypeA(202)); // TTA: C2 M1 D1(temp)
    OneOf<TestTypeA, TestTypeB> oo2(TestTypeB("initial_b")); // TTB: C2 M1 D1(temp)

    TestTypeA::reset_counts(); // Reset A counts after oo1 setup
    TestTypeB::reset_counts(); // Reset B counts after oo2 setup
    // Current state: oo1 has A(202), oo2 has B("initial_b")
    // TTA counts: C0 M0 D0. TTB counts: C0 M0 D0.

    oo2 = oo1; // B in oo2 destructs (TTB D=1). A from oo1 copy-constructs into oo2 (TTA C=1, Cp=1).
    ASSERT_TRUE(oo2.has<TestTypeA>());
    ASSERT_EQ(oo2.get_if<TestTypeA>()->id, 202);
    ASSERT_EQ(TestTypeA::constructions, 1);
    ASSERT_EQ(TestTypeA::copies, 1);
    ASSERT_EQ(TestTypeB::destructions, 1);
}

TEST_F(OneOfTest, MoveConstruction) {
    {
        OneOf<TestTypeA, TestTypeB> oo1(TestTypeA(303)); // TTA: C2 M1 D1(temp)
        ASSERT_EQ(TestTypeA::constructions, 2);
        ASSERT_EQ(TestTypeA::moves, 1);
        ASSERT_EQ(TestTypeA::destructions, 1);

        OneOf<TestTypeA, TestTypeB> oo2 = std::move(oo1); // A from oo1 moved to oo2. TTA: C=2+1, M=1+1. oo1 valueless.

        ASSERT_EQ(TestTypeA::constructions, 3);
        ASSERT_EQ(TestTypeA::moves, 2);
        ASSERT_EQ(TestTypeA::destructions, 1); // Still only tmp TTA from oo1's construction
        ASSERT_TRUE(oo2.has<TestTypeA>());
        ASSERT_EQ(oo2.get_if<TestTypeA>()->id, 303);
        ASSERT_EQ(oo1.get_if<TestTypeA>(), nullptr);
        ASSERT_FALSE(oo1.has_value());
    } // oo2 destructs (A(303) from it). TTA D = 1+1=2. oo1 (valueless) destructs.
    ASSERT_EQ(TestTypeA::destructions, 2);
}


TEST_F(OneOfTest, MoveAssignment) {
    { // Scope for oo1 and oo2 to check destruction
        OneOf<TestTypeA, TestTypeB> oo1(TestTypeA(404)); // TTA: C2 M1 D1(temp for A(404))
        OneOf<TestTypeA, TestTypeB> oo2(TestTypeB("move_assign_test")); // TTB: C2 M1 D1(temp for B)

        TestTypeA::reset_counts(); // Counts for A relative to this point
        TestTypeB::reset_counts(); // Counts for B relative to this point

        oo2 = std::move(oo1); // B in oo2 destructs (TTB D=1). A from oo1 moves to oo2 (TTA C=1 M=1). oo1 valueless.

        ASSERT_TRUE(oo2.has<TestTypeA>());
        ASSERT_EQ(oo2.get_if<TestTypeA>()->id, 404);
        ASSERT_FALSE(oo1.has_value());
        ASSERT_EQ(TestTypeA::constructions, 1);
        ASSERT_EQ(TestTypeA::moves, 1);
        ASSERT_EQ(TestTypeB::destructions, 1);
    } // oo2 destructs (A(404) from it, TTA D=1). oo1 (valueless) destructs.
      // Original A(404) from outer scope's oo1 already destructed.
      // Original B("move_assign_test") from outer scope's oo2 already destructed.
    ASSERT_EQ(TestTypeA::destructions, 1); // The TTA from the moved-into oo2
    ASSERT_EQ(TestTypeB::destructions, 1); // The TTB that was originally in oo2
}

TEST_F(OneOfTest, StdStringOperations) {
    OneOf<int, std::string, double> oo(std::string("hello std::string"));
    ASSERT_TRUE(oo.has<std::string>());
    ASSERT_NE(oo.get_if<std::string>(), nullptr);
    ASSERT_EQ(*oo.get_if<std::string>(), "hello std::string");
    ASSERT_EQ(oo.index(), 1);
    ASSERT_EQ(oo.type(), std::type_index(typeid(std::string)));
    oo.set(std::string("another string"));
    ASSERT_EQ(*oo.get_if<std::string>(), "another string");
    oo.emplace<std::string>("emplaced string");
    ASSERT_EQ(*oo.get_if<std::string>(), "emplaced string");
}

TEST_F(OneOfTest, StdStringVisiting) {
    OneOf<int, std::string> oo(std::string("visiting"));
    struct StringVisitor {
        mutable std::string val;
        void operator()(const std::string& s) const { val = s; }
        void operator()(std::string& s) { s += " visited"; val = s; }
        void operator()(int) const {}
        void operator()(int&) {}
    } visitor;
    oo.visit(visitor);
    ASSERT_EQ(visitor.val, "visiting visited");
    ASSERT_EQ(*oo.get_if<std::string>(), "visiting visited");
    const auto& const_oo = oo;
    visitor.val = "";
    const_oo.visit(visitor);
    ASSERT_EQ(visitor.val, "visiting visited");
}

TEST_F(OneOfTest, MoveOnlyConstructionRvalue) {
    {
        OneOf<MoveOnlyType, int> oo(MoveOnlyType(10)); // MOT tmp(10) C1. MOT in oo C2 M1. tmp D1.
        ASSERT_TRUE(oo.has<MoveOnlyType>());
        ASSERT_EQ(oo.get_if<MoveOnlyType>()->id, 10);
        ASSERT_EQ(MoveOnlyType::constructions, 2);
        ASSERT_EQ(MoveOnlyType::moves, 1);
        ASSERT_EQ(MoveOnlyType::destructions, 1);
    } // MOT in oo D2.
    ASSERT_EQ(MoveOnlyType::destructions, 2);
}

TEST_F(OneOfTest, MoveOnlyEmplace) {
    {
        OneOf<MoveOnlyType, int> oo;
        oo.emplace<MoveOnlyType>(20); // MOT in oo C1.
        ASSERT_TRUE(oo.has<MoveOnlyType>());
        ASSERT_EQ(oo.get_if<MoveOnlyType>()->id, 20);
        ASSERT_EQ(MoveOnlyType::constructions, 1);
        ASSERT_EQ(MoveOnlyType::moves, 0);
    } // MOT in oo D1.
    ASSERT_EQ(MoveOnlyType::destructions, 1);
}

TEST_F(OneOfTest, MoveOnlySetRvalue) {
    {
        OneOf<MoveOnlyType, int> oo;
        oo.set(MoveOnlyType(30)); // MOT tmp(30) C1. MOT in oo C2 M1. tmp D1.
        ASSERT_TRUE(oo.has<MoveOnlyType>());
        ASSERT_EQ(oo.get_if<MoveOnlyType>()->id, 30);
        ASSERT_EQ(MoveOnlyType::constructions, 2);
        ASSERT_EQ(MoveOnlyType::moves, 1);
        ASSERT_EQ(MoveOnlyType::destructions, 1);

        oo.set(MoveOnlyType(40)); // MOT in oo (30) D2. MOT tmp(40) C3. MOT in oo C4 M2. tmp D3.
        ASSERT_EQ(oo.get_if<MoveOnlyType>()->id, 40);
        ASSERT_EQ(MoveOnlyType::constructions, 4);
        ASSERT_EQ(MoveOnlyType::moves, 2);
        ASSERT_EQ(MoveOnlyType::destructions, 3);
    } // MOT in oo (40) D4.
    ASSERT_EQ(MoveOnlyType::destructions, 4);
}

TEST_F(OneOfTest, MoveOnlyMoveConstructionOneOf) {
    {
        OneOf<MoveOnlyType, int> oo1(MoveOnlyType(50)); // MOT tmp C1. MOT oo1 C2 M1. tmp D1.
        ASSERT_EQ(MoveOnlyType::constructions, 2);
        ASSERT_EQ(MoveOnlyType::moves, 1);
        ASSERT_EQ(MoveOnlyType::destructions, 1);

        OneOf<MoveOnlyType, int> oo2 = std::move(oo1); // MOT oo2 C3 M2. oo1 valueless.
        ASSERT_TRUE(oo2.has<MoveOnlyType>());
        ASSERT_EQ(oo2.get_if<MoveOnlyType>()->id, 50);
        ASSERT_FALSE(oo1.has_value());
        ASSERT_EQ(MoveOnlyType::constructions, 3);
        ASSERT_EQ(MoveOnlyType::moves, 2);
        ASSERT_EQ(MoveOnlyType::destructions, 1);
    } // MOT oo2 D2.
    ASSERT_EQ(MoveOnlyType::destructions, 2);
}

TEST_F(OneOfTest, MoveOnlyMoveAssignmentOneOf) {
    OneOf<MoveOnlyType, int> oo1_outer(MoveOnlyType(60)); // MOT tmp C1. MOT oo1_outer C2 M1. tmp D1.
    {
        OneOf<MoveOnlyType, int> oo2;
        oo2.emplace<int>(123); // oo2 has int.

        // MOT counts for this specific assignment phase
        MoveOnlyType::reset_counts(); // Reset for MOTs involved in assign from oo1_outer to oo2

        oo2 = std::move(oo1_outer); // int in oo2 destructs. MOT from oo1_outer moves to oo2. MOT C1 M1. oo1_outer valueless.
        ASSERT_TRUE(oo2.has<MoveOnlyType>());
        ASSERT_EQ(oo2.get_if<MoveOnlyType>()->id, 60);
        ASSERT_FALSE(oo1_outer.has_value());
        ASSERT_EQ(MoveOnlyType::constructions, 1);
        ASSERT_EQ(MoveOnlyType::moves, 1);
        ASSERT_EQ(MoveOnlyType::destructions, 0);
    } // oo2 destructs (MOT 60). MOT D1.
    ASSERT_EQ(MoveOnlyType::destructions, 1);
    // oo1_outer (valueless) also destructs here. The original MOT tmp(60) and MOT in oo1_outer (before move) were from before reset_counts.
}


TEST(OneOfMetaTest, TypeHelperMetafunctions) {
    ASSERT_TRUE((type_index_in_pack_v<int, int, float, char> == 0));
    ASSERT_TRUE((type_index_in_pack_v<float, int, float, char> == 1));
    ASSERT_TRUE((type_index_in_pack_v<char, int, float, char> == 2));

    ASSERT_TRUE((is_one_of_types_v<int, int, float, char>));
    ASSERT_TRUE((is_one_of_types_v<float, int, float, char>));
    ASSERT_TRUE((is_one_of_types_v<char, int, float, char>));
    ASSERT_FALSE((is_one_of_types_v<double, int, float, char>));
    ASSERT_FALSE((is_one_of_types_v<int>));
}

TEST_F(OneOfTest, SingleType) {
    OneOf<int> oo_int(123);
    ASSERT_TRUE(oo_int.has<int>());
    ASSERT_EQ(*oo_int.get_if<int>(), 123);
    ASSERT_EQ(oo_int.index(), 0);
    struct VisitorSingle {
        mutable int val = 0;
        void operator()(int v) const { val = v; }
    } visitor;
    oo_int.visit(visitor);
    ASSERT_EQ(visitor.val, 123);

    TestTypeA::reset_counts();
    {
        OneOf<TestTypeA> oo_a(TestTypeA(77)); // TTA tmp C1. TTA oo_a C2 M1. tmp D1.
        ASSERT_TRUE(oo_a.has<TestTypeA>());
        ASSERT_EQ(oo_a.get_if<TestTypeA>()->id, 77);
        ASSERT_EQ(TestTypeA::constructions, 2);
        ASSERT_EQ(TestTypeA::moves, 1);
        ASSERT_EQ(TestTypeA::destructions, 1);
    } // TTA oo_a D2.
    ASSERT_EQ(TestTypeA::destructions, 2);
}

TEST_F(OneOfTest, ConstCorrectness) {
    OneOf<TestTypeA, int> oo(TestTypeA(10)); // TTA tmp C1. TTA oo C2 M1. tmp D1.
    const OneOf<TestTypeA, int>& const_oo = oo;
    ASSERT_TRUE(const_oo.has<TestTypeA>());
    ASSERT_NE(const_oo.get_if<TestTypeA>(), nullptr);
    ASSERT_EQ(const_oo.get_if<TestTypeA>()->id, 10);
    ASSERT_EQ(const_oo.index(), 0);
    ASSERT_TRUE(const_oo.has_value());
    ASSERT_EQ(const_oo.type(), std::type_index(typeid(TestTypeA)));
    TestVisitor visitor;
    const_oo.visit(visitor); // Calls const visitor
    ASSERT_EQ(visitor.visited_a, 1);
    ASSERT_EQ(visitor.last_a_id, 10);
    ASSERT_EQ(oo.get_if<TestTypeA>()->id, 10); // Ensure original not modified
}
