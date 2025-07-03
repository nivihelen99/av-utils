#include "gtest/gtest.h"
#include "../include/tagged_ptr.h" // Adjust path as necessary based on include directories
#include <cstdint> // For uintptr_t, uint8_t
#include <cstddef> // For size_t

// Define some test structures with specific alignments
struct alignas(8) AlignedStruct8 {
    int data;
    char c;
    bool operator==(const AlignedStruct8& other) const {
        return data == other.data && c == other.c;
    }
};

struct alignas(16) AlignedStruct16 {
    long long data;
    char c;
     bool operator==(const AlignedStruct16& other) const {
        return data == other.data && c == other.c;
    }
};

// This struct is intentionally misaligned for some tests if TagBits > 2
struct alignas(4) AlignedStruct4 {
    int data;
     bool operator==(const AlignedStruct4& other) const {
        return data == other.data;
    }
};

// Test fixture for TaggedPtr tests
class TaggedPtrTest : public ::testing::Test {
protected:
    AlignedStruct8 obj8_1, obj8_2;
    AlignedStruct16 obj16_1, obj16_2;
    // AlignedStruct4 obj4_1; // Used for compile-time tests manually
};

TEST_F(TaggedPtrTest, BasicEncodingDecoding) {
    TaggedPtr<AlignedStruct8, 3> tp; // 3 bits for tag, alignof(AlignedStruct8) == 8

    tp.set(&obj8_1, 0);
    EXPECT_EQ(tp.get_ptr(), &obj8_1);
    EXPECT_EQ(tp.get_tag(), 0);

    tp.set(&obj8_2, 5); // 5 is 101 in binary
    EXPECT_EQ(tp.get_ptr(), &obj8_2);
    EXPECT_EQ(tp.get_tag(), 5);
}

TEST_F(TaggedPtrTest, ConstructorInitialization) {
    TaggedPtr<AlignedStruct8, 3> tp(&obj8_1, 7);
    EXPECT_EQ(tp.get_ptr(), &obj8_1);
    EXPECT_EQ(tp.get_tag(), 7);
}

TEST_F(TaggedPtrTest, NullPtrHandling) {
    TaggedPtr<AlignedStruct16, 4> tp;

    tp.set(nullptr, 3);
    EXPECT_EQ(tp.get_ptr(), nullptr);
    EXPECT_EQ(tp.get_tag(), 3);

    tp.set_ptr(nullptr); // Should preserve tag
    EXPECT_EQ(tp.get_ptr(), nullptr);
    EXPECT_EQ(tp.get_tag(), 3);

    TaggedPtr<AlignedStruct16, 4> tp_with_null_constructor(nullptr, 10);
    EXPECT_EQ(tp_with_null_constructor.get_ptr(), nullptr);
    EXPECT_EQ(tp_with_null_constructor.get_tag(), 10);
}

TEST_F(TaggedPtrTest, TagOperations) {
    TaggedPtr<AlignedStruct16, 4> tp(&obj16_1, 1); // 4 bits for tag

    EXPECT_EQ(tp.get_ptr(), &obj16_1);
    EXPECT_EQ(tp.get_tag(), 1);

    tp.set_tag(10); // Max tag is 15 (2^4 - 1)
    EXPECT_EQ(tp.get_ptr(), &obj16_1); // Pointer preserved
    EXPECT_EQ(tp.get_tag(), 10);

    tp.set_tag(15); // Max tag
    EXPECT_EQ(tp.get_tag(), 15);

    // Test tag truncation (current implementation truncates)
    // TagBits = 4, so max_tag = 15. 16 (10000) should be truncated to 0 (0000).
    tp.set_tag(16);
    EXPECT_EQ(tp.get_tag(), 0);

    tp.set_tag(TaggedPtr<AlignedStruct16, 4>::max_tag());
    EXPECT_EQ(tp.get_tag(), (TaggedPtr<AlignedStruct16, 4>::max_tag()));
    EXPECT_EQ(tp.get_tag(), 15);
}

TEST_F(TaggedPtrTest, PointerOperations) {
    TaggedPtr<AlignedStruct8, 2> tp(&obj8_1, 3); // 2 bits for tag

    EXPECT_EQ(tp.get_ptr(), &obj8_1);
    EXPECT_EQ(tp.get_tag(), 3);

    tp.set_ptr(&obj8_2);
    EXPECT_EQ(tp.get_ptr(), &obj8_2); // Pointer updated
    EXPECT_EQ(tp.get_tag(), 3);       // Tag preserved
}

TEST_F(TaggedPtrTest, RawValueConversion) {
    TaggedPtr<AlignedStruct16, 3> tp1(&obj16_1, 5);
    uintptr_t raw_val = tp1.as_uintptr_t();

    TaggedPtr<AlignedStruct16, 3> tp2 = TaggedPtr<AlignedStruct16, 3>::from_raw(raw_val);
    EXPECT_EQ(tp1.get_ptr(), tp2.get_ptr());
    EXPECT_EQ(tp1.get_tag(), tp2.get_tag());
    EXPECT_EQ(tp1, tp2); // Uses operator==

    // Check with nullptr
    tp1.set(nullptr, 2);
    raw_val = tp1.as_uintptr_t();
    tp2 = TaggedPtr<AlignedStruct16, 3>::from_raw(raw_val);
    EXPECT_EQ(tp2.get_ptr(), nullptr);
    EXPECT_EQ(tp2.get_tag(), 2);
    EXPECT_EQ(tp1, tp2);
}

TEST_F(TaggedPtrTest, ComparisonOperators) {
    TaggedPtr<AlignedStruct8, 1> tp1(&obj8_1, 0);
    TaggedPtr<AlignedStruct8, 1> tp2(&obj8_1, 0); // Same as tp1
    TaggedPtr<AlignedStruct8, 1> tp3(&obj8_1, 1); // Same ptr, different tag
    TaggedPtr<AlignedStruct8, 1> tp4(&obj8_2, 0); // Different ptr, same tag
    TaggedPtr<AlignedStruct8, 1> tp_null1(nullptr, 0);
    TaggedPtr<AlignedStruct8, 1> tp_null2(nullptr, 0); // Same as tp_null1
    TaggedPtr<AlignedStruct8, 1> tp_null_tag(nullptr, 1); // Nullptr, different tag

    EXPECT_TRUE(tp1 == tp2);
    EXPECT_FALSE(tp1 != tp2);

    EXPECT_TRUE(tp1 != tp3);
    EXPECT_FALSE(tp1 == tp3);

    EXPECT_TRUE(tp1 != tp4);
    EXPECT_FALSE(tp1 == tp4);

    EXPECT_TRUE(tp_null1 == tp_null2);
    EXPECT_FALSE(tp_null1 != tp_null2);

    EXPECT_TRUE(tp_null1 != tp_null_tag);
    EXPECT_FALSE(tp_null1 == tp_null_tag);
}

TEST_F(TaggedPtrTest, MaxTagValue) {
    EXPECT_EQ((TaggedPtr<AlignedStruct8, 1>::max_tag()), 1);  // 2^1 - 1
    EXPECT_EQ((TaggedPtr<AlignedStruct8, 2>::max_tag()), 3);  // 2^2 - 1
    EXPECT_EQ((TaggedPtr<AlignedStruct8, 3>::max_tag()), 7);  // 2^3 - 1
    EXPECT_EQ((TaggedPtr<AlignedStruct16, 4>::max_tag()), 15); // 2^4 - 1
}

// Test for alignment static_assert.
// This test doesn't run; its purpose is to fail compilation if static_assert works.
// To verify:
// 1. Uncomment the line inside the function.
// 2. Try to compile. It should fail with a static_assert message.
// 3. Comment it back to allow other tests to compile and run.
/*
TEST_F(TaggedPtrTest, CompileTimeAlignmentCheck) {
    // AlignedStruct4 has alignof(4). Requesting 3 tag bits requires 2^3 = 8 byte alignment.
    // This line should cause a static_assert to trigger during compilation.
    // TaggedPtr<AlignedStruct4, 3> tp_compile_fail;
    // (void)tp_compile_fail; // Suppress unused variable warning if it were to compile
}
*/

// It might also be useful to test with void* if the TaggedPtr is intended for such use.
// For example, TaggedPtr<void, 3> could point to any type of data.
// The current static_assert relies on alignof(T), which is problematic for T=void.
// A specialization or SFINAE might be needed for TaggedPtr<void, TagBits>.
// For now, assuming T is a concrete, alignable type.

TEST_F(TaggedPtrTest, ZeroTagBits) {
    // What happens if TagBits is 0?
    // static_assert(TagBits < sizeof(uintptr_t) * 8, ...) - passes
    // static_assert(alignof(T) >= (1 << TagBits), ...) -> alignof(T) >= (1 << 0) -> alignof(T) >= 1. This is always true.
    // TAG_MASK = (1 << 0) - 1 = 1 - 1 = 0
    // POINTER_MASK = ~0 = all 1s.
    TaggedPtr<AlignedStruct8, 0> tp(&obj8_1, 123); // Tag should be ignored/masked to 0

    EXPECT_EQ(tp.get_ptr(), &obj8_1);
    EXPECT_EQ(tp.get_tag(), 0); // Tag is masked out
    EXPECT_EQ((TaggedPtr<AlignedStruct8, 0>::max_tag()), 0);

    tp.set_tag(42); // Should have no effect on the tag, it remains 0
    EXPECT_EQ(tp.get_tag(), 0);

    uintptr_t raw = tp.as_uintptr_t();
    EXPECT_EQ(raw, reinterpret_cast<uintptr_t>(&obj8_1)); // Raw value is just the pointer

    TaggedPtr<AlignedStruct8, 0> tp2 = TaggedPtr<AlignedStruct8, 0>::from_raw(reinterpret_cast<uintptr_t>(&obj8_2));
    EXPECT_EQ(tp2.get_ptr(), &obj8_2);
    EXPECT_EQ(tp2.get_tag(), 0);
}

// Main function for Google Test is usually not needed here if linked with GTest::gtest_main
// int main(int argc, char **argv) {
//     ::testing::InitGoogleTest(&argc, argv);
//     return RUN_ALL_TESTS();
// }
