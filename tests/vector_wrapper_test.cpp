#include "vector_wrapper.h" // Adjust path as necessary
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <stdexcept> // For std::out_of_range
#include <list>      // For testing with different underlying container

// Test fixture for VectorWrapper
class VectorWrapperTest : public ::testing::Test {
protected:
    VectorWrapper<int> vec;
    VectorWrapper<std::string> str_vec;
};

// Test constructors
TEST_F(VectorWrapperTest, Constructors) {
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(0, vec.size());

    VectorWrapper<int> vec_init_list = {1, 2, 3};
    EXPECT_FALSE(vec_init_list.empty());
    EXPECT_EQ(3, vec_init_list.size());
    EXPECT_EQ(1, vec_init_list[0]);
    EXPECT_EQ(2, vec_init_list[1]);
    EXPECT_EQ(3, vec_init_list[2]);

    VectorWrapper<int> vec_count_val(5, 10); // 5 elements, all value 10
    EXPECT_EQ(5, vec_count_val.size());
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(10, vec_count_val[i]);
    }

    VectorWrapper<int> vec_count(3); // 3 default-constructed elements
    EXPECT_EQ(3, vec_count.size());
     for (size_t i = 0; i < 3; ++i) {
        EXPECT_EQ(0, vec_count[i]); // int() is 0
    }

    std::vector<int> source_vec = {4, 5, 6};
    VectorWrapper<int> vec_from_iter(source_vec.begin(), source_vec.end());
    EXPECT_EQ(3, vec_from_iter.size());
    EXPECT_EQ(4, vec_from_iter[0]);
    EXPECT_EQ(6, vec_from_iter[2]);

    VectorWrapper<int> vec_copy(vec_init_list);
    EXPECT_EQ(3, vec_copy.size());
    EXPECT_EQ(1, vec_copy[0]);

    VectorWrapper<int> vec_move(std::move(vec_init_list));
    EXPECT_EQ(3, vec_move.size());
    EXPECT_EQ(1, vec_move[0]);
    // Check original vec_init_list is in a valid but unspecified state (likely empty for vector move)
    // For std::vector, it's typically empty after move.
    // EXPECT_TRUE(vec_init_list.empty()); // This depends on InnerContainer's move constructor
}

// Test assignment operators
TEST_F(VectorWrapperTest, AssignmentOperators) {
    VectorWrapper<int> v1 = {1, 2};
    VectorWrapper<int> v2;
    v2 = v1; // Copy assignment
    EXPECT_EQ(2, v2.size());
    EXPECT_EQ(1, v2[0]);

    VectorWrapper<int> v3;
    v3 = std::move(v1); // Move assignment
    EXPECT_EQ(2, v3.size());
    EXPECT_EQ(1, v3[0]);
    // EXPECT_TRUE(v1.empty()); // Original v1 state

    VectorWrapper<int> v4;
    v4 = {7, 8, 9}; // Initializer list assignment
    EXPECT_EQ(3, v4.size());
    EXPECT_EQ(7, v4[0]);
}

// Test assign methods
TEST_F(VectorWrapperTest, AssignMethods) {
    vec.assign(3, 100); // count, value
    EXPECT_EQ(3, vec.size());
    EXPECT_EQ(100, vec[0]);

    std::list<int> l = {200, 300};
    vec.assign(l.begin(), l.end()); // iterators
    EXPECT_EQ(2, vec.size());
    EXPECT_EQ(200, vec[0]);

    vec.assign({1, 2, 3, 4}); // initializer_list
    EXPECT_EQ(4, vec.size());
    EXPECT_EQ(1, vec[0]);
}

// Test element access
TEST_F(VectorWrapperTest, ElementAccess) {
    vec.push_back(10);
    vec.push_back(20);
    EXPECT_EQ(10, vec.at(0));
    EXPECT_EQ(20, vec.at(1));
    EXPECT_EQ(10, vec[0]);
    EXPECT_EQ(20, vec[1]);
    EXPECT_EQ(10, vec.front());
    EXPECT_EQ(20, vec.back());

    const auto& const_vec = vec;
    EXPECT_EQ(10, const_vec.at(0));
    EXPECT_EQ(10, const_vec[0]);
    EXPECT_EQ(10, const_vec.front());
    EXPECT_EQ(20, const_vec.back());

    EXPECT_THROW(vec.at(2), std::out_of_range);
    // operator[] out of bounds is UB, not typically tested for throwing

    int* p_data = vec.data_ptr();
    EXPECT_EQ(10, p_data[0]);
    const int* p_const_data = const_vec.data_ptr();
    EXPECT_EQ(10, p_const_data[0]);
}

// Test iterators
TEST_F(VectorWrapperTest, Iterators) {
    vec = {1, 2, 3};
    int sum = 0;
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        sum += *it;
    }
    EXPECT_EQ(6, sum);

    sum = 0;
    for (auto it = vec.cbegin(); it != vec.cend(); ++it) {
        sum += *it;
    }
    EXPECT_EQ(6, sum);

    sum = 0;
    for (const auto& val : vec) { // Range-based for
        sum += val;
    }
    EXPECT_EQ(6, sum);

    sum = 0;
    for (auto it = vec.rbegin(); it != vec.rend(); ++it) {
        sum += *it;
    }
    EXPECT_EQ(6, sum);
    EXPECT_EQ(3, *vec.rbegin());
    EXPECT_EQ(1, *(vec.rend()-1));


    const auto& const_vec = vec;
    sum = 0;
    for (auto it = const_vec.begin(); it != const_vec.end(); ++it) {
        sum += *it;
    }
    EXPECT_EQ(6, sum);
}

// Test capacity
TEST_F(VectorWrapperTest, Capacity) {
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(0, vec.size());
    // max_size is implementation-defined, just check it's non-zero
    EXPECT_GT(vec.max_size(), 0);

    vec.reserve(100);
    EXPECT_GE(vec.capacity(), 100);
    EXPECT_EQ(0, vec.size()); // Reserve doesn't change size

    vec.push_back(1);
    vec.push_back(2);
    EXPECT_FALSE(vec.empty());
    EXPECT_EQ(2, vec.size());

    vec.reserve(1); // Reserve smaller than current capacity might not change it
    size_t cap_before_shrink = vec.capacity();
    vec.shrink_to_fit();
    EXPECT_LE(vec.capacity(), cap_before_shrink);
    EXPECT_EQ(2, vec.size()); // shrink_to_fit doesn't change size
}

// Test modifiers
TEST_F(VectorWrapperTest, Modifiers) {
    // clear
    vec = {1, 2, 3};
    ASSERT_FALSE(vec.empty());
    vec.clear();
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(0, vec.size());

    // insert
    vec.insert(vec.begin(), 10); // {10}
    EXPECT_EQ(1, vec.size());
    EXPECT_EQ(10, vec[0]);

    vec.insert(vec.end(), 30); // {10, 30}
    EXPECT_EQ(30, vec[1]);

    vec.insert(vec.begin() + 1, 20); // {10, 20, 30}
    EXPECT_EQ(20, vec[1]);
    EXPECT_EQ(3, vec.size());

    vec.insert(vec.begin(), 2, 5); // {5, 5, 10, 20, 30}
    EXPECT_EQ(5, vec.size());
    EXPECT_EQ(5, vec[0]);
    EXPECT_EQ(5, vec[1]);

    std::vector<int> source = {0, 0};
    vec.insert(vec.begin(), source.begin(), source.end()); // {0, 0, 5, 5, 10, 20, 30}
    EXPECT_EQ(7, vec.size());
    EXPECT_EQ(0, vec[0]);

    vec.insert(vec.end(), {40, 50}); // {0,0,5,5,10,20,30,40,50}
    EXPECT_EQ(9, vec.size());
    EXPECT_EQ(50, vec.back());

    // emplace
    auto it_emplace = vec.emplace(vec.begin() + 1, 1000); // Emplace 1000 after first 0
    EXPECT_EQ(1000, *it_emplace);
    EXPECT_EQ(0, vec[0]);
    EXPECT_EQ(1000, vec[1]);
    EXPECT_EQ(0, vec[2]);
    EXPECT_EQ(10, vec.size());

    // emplace_back
    vec.emplace_back(2000);
    EXPECT_EQ(2000, vec.back());
    EXPECT_EQ(11, vec.size());

    // erase
    auto it_erase = vec.erase(vec.begin()); // Erase first 0
    EXPECT_EQ(1000, *it_erase); // Iterator to element after erased one
    EXPECT_EQ(10, vec.size());

    it_erase = vec.erase(vec.begin() + 1, vec.begin() + 3); // Erase 0 and 5
    EXPECT_EQ(5, *it_erase); // Iterator to element after erased range (the second 5)
    EXPECT_EQ(8, vec.size());
    EXPECT_EQ(1000, vec[0]);
    EXPECT_EQ(5, vec[1]); // formerly vec[3]

    // push_back & pop_back
    str_vec.push_back("hello");
    str_vec.push_back("world");
    EXPECT_EQ(2, str_vec.size());
    EXPECT_EQ("world", str_vec.back());
    str_vec.pop_back();
    EXPECT_EQ(1, str_vec.size());
    EXPECT_EQ("hello", str_vec.back());
    str_vec.pop_back();
    EXPECT_TRUE(str_vec.empty());
    // pop_back on empty vector is UB, not tested

    // resize
    vec = {1,2,3};
    vec.resize(5); // {1,2,3,0,0}
    EXPECT_EQ(5, vec.size());
    EXPECT_EQ(0, vec[4]);
    vec.resize(7, 100); // {1,2,3,0,0,100,100}
    EXPECT_EQ(7, vec.size());
    EXPECT_EQ(100, vec[6]);
    vec.resize(2); // {1,2}
    EXPECT_EQ(2, vec.size());
    EXPECT_EQ(2, vec.back());

    // swap
    VectorWrapper<int> v_swap1 = {1,2};
    VectorWrapper<int> v_swap2 = {3,4,5};
    v_swap1.swap(v_swap2);
    EXPECT_EQ(3, v_swap1.size());
    EXPECT_EQ(3, v_swap1[0]);
    EXPECT_EQ(2, v_swap2.size());
    EXPECT_EQ(1, v_swap2[0]);

    swap(v_swap1, v_swap2); // Non-member swap
    EXPECT_EQ(2, v_swap1.size());
    EXPECT_EQ(1, v_swap1[0]);
    EXPECT_EQ(3, v_swap2.size());
    EXPECT_EQ(3, v_swap2[0]);
}

// Test equality operators
TEST_F(VectorWrapperTest, EqualityOperators) {
    VectorWrapper<int> v1 = {1, 2, 3};
    VectorWrapper<int> v2 = {1, 2, 3};
    VectorWrapper<int> v3 = {1, 2, 4};
    VectorWrapper<int> v4 = {1, 2};

    EXPECT_TRUE(v1 == v2);
    EXPECT_FALSE(v1 == v3);
    EXPECT_FALSE(v1 == v4);

    EXPECT_FALSE(v1 != v2);
    EXPECT_TRUE(v1 != v3);
    EXPECT_TRUE(v1 != v4);
}


// --- Tests for derived class overriding behavior ---

// Custom vector for testing override
template <typename T, typename InnerContainer = std::vector<T>>
class TestObservableVector : public VectorWrapper<T, InnerContainer> {
public:
    using Base = VectorWrapper<T, InnerContainer>;
    mutable int push_back_called = 0;
    mutable int pop_back_called = 0;
    mutable int insert_called = 0;
    mutable int erase_called = 0;
    mutable int at_called = 0;
    mutable int bracket_called = 0;
    mutable int clear_called = 0;

    using Base::Base; // Inherit constructors

    void push_back(const T& value) override {
        push_back_called++;
        Base::push_back(value);
    }
    void push_back(T&& value) override {
        push_back_called++;
        Base::push_back(std::move(value));
    }
    void pop_back() override {
        pop_back_called++;
        Base::pop_back();
    }
    // Corrected: Use typename Base::iterator and typename Base::const_iterator
    typename Base::iterator insert(typename Base::const_iterator pos, const T& value) override {
        insert_called++;
        return Base::insert(pos, value);
    }
    // Corrected: Use typename Base::iterator and typename Base::const_iterator
    typename Base::iterator erase(typename Base::const_iterator pos) override {
        erase_called++;
        return Base::erase(pos);
    }
     typename Base::reference at(typename Base::size_type n) override {
        at_called++;
        return Base::at(n);
    }
    typename Base::const_reference at(typename Base::size_type n) const override {
        at_called++;
        return Base::at(n);
    }
    typename Base::reference operator[](typename Base::size_type n) override {
        bracket_called++;
        return Base::operator[](n);
    }
    typename Base::const_reference operator[](typename Base::size_type n) const override {
        bracket_called++;
        return Base::operator[](n);
    }
    void clear() noexcept override {
        clear_called++;
        Base::clear();
    }
};

TEST_F(VectorWrapperTest, DerivedClassOverrides) {
    TestObservableVector<int> obs_vec;

    obs_vec.push_back(10);
    EXPECT_EQ(1, obs_vec.push_back_called);
    EXPECT_EQ(10, obs_vec[0]); // Calls overridden operator[]
    EXPECT_EQ(1, obs_vec.bracket_called);
    obs_vec.bracket_called = 0; // Reset for next specific test

    obs_vec.push_back(20);
    EXPECT_EQ(2, obs_vec.push_back_called);
    EXPECT_EQ(20, obs_vec.at(1)); // Calls overridden at()
    EXPECT_EQ(1, obs_vec.at_called);
    obs_vec.at_called = 0; // Reset

    obs_vec.insert(obs_vec.begin(), 5);
    EXPECT_EQ(1, obs_vec.insert_called);
    EXPECT_EQ(5, obs_vec.front());

    obs_vec.pop_back(); // Pops 20
    EXPECT_EQ(1, obs_vec.pop_back_called);
    EXPECT_EQ(10, obs_vec.back());

    obs_vec.erase(obs_vec.begin()); // Erases 5
    EXPECT_EQ(1, obs_vec.erase_called);
    EXPECT_EQ(10, obs_vec.front());

    const auto& const_obs_vec = obs_vec;
    EXPECT_EQ(10, const_obs_vec.at(0));
    EXPECT_EQ(1, obs_vec.at_called); // at_called is mutable
    obs_vec.at_called = 0;

    [[maybe_unused]] int val = const_obs_vec[0];
    EXPECT_EQ(1, obs_vec.bracket_called);
    obs_vec.bracket_called = 0;

    obs_vec.clear();
    EXPECT_EQ(1, obs_vec.clear_called);
    EXPECT_TRUE(obs_vec.empty());
}

// Test with a different inner container (std::list)
// Note: Some operations might not be available or have different complexities.
// This is a basic test to ensure the wrapper concept works with other containers.
TEST_F(VectorWrapperTest, DifferentInnerContainer) {
    VectorWrapper<int, std::list<int>> list_wrapper;
    list_wrapper.push_back(1);
    list_wrapper.push_back(2);
    ASSERT_EQ(2, list_wrapper.size());
    EXPECT_EQ(1, list_wrapper.front());
    EXPECT_EQ(2, list_wrapper.back());

    list_wrapper.pop_back();
    EXPECT_EQ(1, list_wrapper.size());
    EXPECT_EQ(1, list_wrapper.front());

    // std::list doesn't have operator[], at(), capacity(), reserve(), shrink_to_fit()
    // So, VectorWrapper with std::list would fail to compile if these were called
    // unless those methods were conditionally enabled (SFINAE), which they are not
    // in the current simple VectorWrapper.
    // This test primarily checks that methods common to vector-like and list-like
    // interfaces (push_back, pop_back, front, back, size, empty, clear, iterators) work.
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
