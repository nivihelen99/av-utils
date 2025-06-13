#include "gtest/gtest.h"
#include "default_dict.h" // Should be found via include_directories in CMake
#include <string>
#include <vector>
#include <memory>      // For std::unique_ptr, std::move_if_noexcept etc.
#include <functional>  // For std::function, std::reference_wrapper
#include <ostream>     // For std::ostream with MyStruct

// Use the namespace for convenience in tests
using namespace std_ext;

// Helper: A simple struct for testing non-primitive types
struct MyStruct {
    int id;
    std::string data;

    // Default constructor for some tests
    MyStruct() : id(0), data("default_constructed") {}
    MyStruct(int i, std::string s) : id(i), data(std::move(s)) {}

    bool operator==(const MyStruct& other) const {
        return id == other.id && data == other.data;
    }
    // For GTest printing on failure
    friend std::ostream& operator<<(std::ostream& os, const MyStruct& s) {
        return os << "MyStruct{id=" << s.id << ", data=\"" << s.data << "\"}";
    }
};

// Custom factory for MyStruct
MyStruct myStructFactoryFunction() {
    return MyStruct{-1, "factory_default"};
}


// Test fixture for defaultdict
class DefaultDictTest : public ::testing::Test {
protected:
    // Per-test set-up and tear-down logic can go here if needed.
};

// --- Test Cases ---

// Test Constructors
TEST_F(DefaultDictTest, ConstructorDefaultFactory) {
    defaultdict<std::string, int> dd(zero_factory<int>());
    ASSERT_EQ(dd.size(), 0);
    ASSERT_TRUE(dd.empty());
    ASSERT_EQ(dd["test"], 0);
    ASSERT_EQ(dd.size(), 1);
}

TEST_F(DefaultDictTest, ConstructorWithExplicitFactoryLambda) {
    defaultdict<int, std::string> dd([](){ return std::string("lambda_default"); });
    ASSERT_EQ(dd.size(), 0);
    ASSERT_EQ(dd[123], "lambda_default");
    ASSERT_EQ(dd.size(), 1);
}

TEST_F(DefaultDictTest, ConstructorWithExplicitFactoryFunctionPtr) {
    defaultdict<int, MyStruct> dd(myStructFactoryFunction);
    ASSERT_EQ(dd.size(), 0);
    ASSERT_EQ(dd[10].id, -1);
    ASSERT_EQ(dd[10].data, "factory_default");
    ASSERT_EQ(dd.size(), 1);
}


TEST_F(DefaultDictTest, ConstructorInitializerList) {
    defaultdict<std::string, int> dd(zero_factory<int>(), {{"a", 1}, {"b", 2}});
    ASSERT_EQ(dd.size(), 2);
    ASSERT_EQ(dd.at("a"), 1);
    ASSERT_EQ(dd.at("b"), 2);
    ASSERT_EQ(dd["c"], 0); // Default created
    ASSERT_EQ(dd.size(), 3);
}

TEST_F(DefaultDictTest, ConstructorCopy) {
    defaultdict<std::string, int> dd1(zero_factory<int>(), {{"a", 1}});
    dd1["b"];
    ASSERT_EQ(dd1.size(), 2);

    defaultdict<std::string, int> dd2(dd1);
    ASSERT_EQ(dd2.size(), 2);
    ASSERT_EQ(dd2.at("a"), 1);
    ASSERT_EQ(dd2.at("b"), 0);
    ASSERT_EQ(dd2["c"], 0);
    ASSERT_EQ(dd2.size(), 3);

    ASSERT_EQ(dd1.size(), 2);
    ASSERT_EQ(dd1.at("a"), 1);
    ASSERT_EQ(dd1.at("b"), 0);
}

TEST_F(DefaultDictTest, ConstructorMove) {
    defaultdict<std::string, int> dd1(zero_factory<int>(), {{"a", 1}});
    dd1["b"];
    ASSERT_EQ(dd1.size(), 2);

    defaultdict<std::string, int> dd2(std::move(dd1));
    ASSERT_EQ(dd2.size(), 2);
    ASSERT_EQ(dd2.at("a"), 1);
    ASSERT_EQ(dd2.at("b"), 0);
    ASSERT_EQ(dd2["c"], 0);
    ASSERT_EQ(dd2.size(), 3);
}

// Test Operator[] and Default Creation
TEST_F(DefaultDictTest, OperatorSquareBracketsDefaultCreation) {
    defaultdict<std::string, int> counter(zero_factory<int>());
    counter["apple"] += 5;
    ASSERT_EQ(counter.at("apple"), 5);
    ASSERT_EQ(counter["banana"], 0);
    ASSERT_EQ(counter.size(), 2);
}

TEST_F(DefaultDictTest, OperatorSquareBracketsExistingValue) {
    defaultdict<std::string, int> counter(zero_factory<int>());
    counter["apple"] = 10;
    ASSERT_EQ(counter.at("apple"), 10);
    counter["apple"] = 20;
    ASSERT_EQ(counter.at("apple"), 20);
    ASSERT_EQ(counter.size(), 1);
}

TEST_F(DefaultDictTest, OperatorSquareBracketsWithCustomStruct) {
    defaultdict<int, MyStruct> dd(myStructFactoryFunction);
    ASSERT_EQ(dd[100].id, -1);
    ASSERT_EQ(dd[100].data, "factory_default");
    dd[100].data = "modified";
    ASSERT_EQ(dd[100].data, "modified");
    ASSERT_EQ(dd[200].id, -1);
    ASSERT_EQ(dd.size(), 2);
}

// Test 'at' method
TEST_F(DefaultDictTest, AtMethodExisting) {
    defaultdict<std::string, int> dd(zero_factory<int>(), {{"one", 1}});
    ASSERT_EQ(dd.at("one"), 1);
    const auto& cdd = dd;
    ASSERT_EQ(cdd.at("one"), 1);
}

TEST_F(DefaultDictTest, AtMethodNonExisting) {
    defaultdict<std::string, int> dd(zero_factory<int>());
    ASSERT_THROW(dd.at("nonexistent"), std::out_of_range);
    const auto& cdd = dd;
    ASSERT_THROW(cdd.at("nonexistent"), std::out_of_range);
    ASSERT_EQ(dd.size(), 0);
}

// Test 'get' method
TEST_F(DefaultDictTest, GetMethod) {
    defaultdict<std::string, int> dd(zero_factory<int>(), {{"one", 1}});
    ASSERT_EQ(dd.get("one"), 1);
    ASSERT_EQ(dd.size(), 1);
    ASSERT_EQ(dd.get("two"), 0);
    ASSERT_EQ(dd.size(), 1);
}

// Test Capacity methods (empty, size)
TEST_F(DefaultDictTest, Capacity) {
    defaultdict<int, int> dd(zero_factory<int>());
    ASSERT_TRUE(dd.empty());
    ASSERT_EQ(dd.size(), 0);
    dd[1];
    ASSERT_FALSE(dd.empty());
    ASSERT_EQ(dd.size(), 1);
    dd[2];
    ASSERT_EQ(dd.size(), 2);
}

// Test Modifiers (clear, insert, emplace, erase, swap)
TEST_F(DefaultDictTest, Clear) {
    defaultdict<std::string, int> dd(zero_factory<int>(), {{"a",1}, {"b",2}});
    dd["c"];
    ASSERT_FALSE(dd.empty());
    ASSERT_EQ(dd.size(), 3);
    dd.clear();
    ASSERT_TRUE(dd.empty());
    ASSERT_EQ(dd.size(), 0);
    ASSERT_EQ(dd["new_after_clear"], 0);
    ASSERT_EQ(dd.size(), 1);
}

TEST_F(DefaultDictTest, InsertLValue) {
    defaultdict<std::string, int> dd(zero_factory<int>());
    std::pair<const std::string, int> val = {"hello", 100};
    auto result = dd.insert(val);
    ASSERT_TRUE(result.second);
    ASSERT_NE(result.first, dd.end());
    ASSERT_EQ(result.first->first, "hello");
    ASSERT_EQ(result.first->second, 100);
    ASSERT_EQ(dd.at("hello"), 100);

    auto result_again = dd.insert(val);
    ASSERT_FALSE(result_again.second);
    ASSERT_NE(result_again.first, dd.end());
    ASSERT_EQ(result_again.first->second, 100);
}

TEST_F(DefaultDictTest, InsertRValue) {
    defaultdict<std::string, int> dd(zero_factory<int>());
    auto result = dd.insert({"world", 200});
    ASSERT_TRUE(result.second);
    ASSERT_NE(result.first, dd.end());
    ASSERT_EQ(result.first->second, 200);
    ASSERT_EQ(dd.at("world"), 200);
}

TEST_F(DefaultDictTest, Emplace) {
    defaultdict<std::string, MyStruct> dd(myStructFactoryFunction);
    auto result = dd.emplace("key1", MyStruct{1, "data1"});
    ASSERT_TRUE(result.second);
    ASSERT_NE(result.first, dd.end());
    ASSERT_EQ(result.first->second.id, 1);
    ASSERT_EQ(dd.at("key1").id, 1);

    auto result_again = dd.emplace("key1", MyStruct{2, "data2"});
    ASSERT_FALSE(result_again.second);
    ASSERT_NE(result_again.first, dd.end());
    ASSERT_EQ(result_again.first->second.id, 1);
}

TEST_F(DefaultDictTest, TryEmplace) {
    defaultdict<std::string, int> dd(zero_factory<int>());
    auto p = dd.try_emplace("new_key", 100);
    ASSERT_TRUE(p.second);
    ASSERT_NE(p.first, dd.end());
    ASSERT_EQ(p.first->second, 100);
    ASSERT_EQ(dd.at("new_key"), 100);

    p = dd.try_emplace("new_key", 200);
    ASSERT_FALSE(p.second);
    ASSERT_NE(p.first, dd.end());
    ASSERT_EQ(p.first->second, 100);
}


TEST_F(DefaultDictTest, InsertOrAssign) {
    defaultdict<std::string, int> dd(zero_factory<int>());
    auto result = dd.insert_or_assign("key", 1);
    ASSERT_TRUE(result.second);
    ASSERT_EQ(dd.at("key"), 1);

    result = dd.insert_or_assign("key", 2);
    ASSERT_FALSE(result.second);
    ASSERT_EQ(dd.at("key"), 2);
}

TEST_F(DefaultDictTest, EraseByKey) {
    defaultdict<std::string, int> dd(zero_factory<int>(), {{"a",1}, {"b",2}, {"c",3}});
    ASSERT_EQ(dd.erase("b"), 1);
    ASSERT_EQ(dd.size(), 2);
    ASSERT_FALSE(dd.contains("b"));
    ASSERT_THROW(dd.at("b"), std::out_of_range);
    ASSERT_EQ(dd.erase("nonexistent"), 0);
    ASSERT_EQ(dd.size(), 2);
}

TEST_F(DefaultDictTest, EraseByIterator) {
    defaultdict<std::string, int> dd(zero_factory<int>(), {{"a",1}, {"b",2}});
    auto it_a = dd.find("a");
    ASSERT_NE(it_a, dd.end());
    auto next_it = dd.erase(it_a);
    ASSERT_EQ(dd.size(), 1);
    ASSERT_FALSE(dd.contains("a"));
    // The returned iterator next_it might be end() if the erased element was last in iteration order.
    // The primary check is that the element is gone and size is correct.
    // If needed, one can check for the presence of "b" explicitly.
    ASSERT_TRUE(dd.contains("b"));
    ASSERT_EQ(dd.at("b"), 2);
}

TEST_F(DefaultDictTest, Swap) {
    auto factory1 = zero_factory<int>();
    auto factory2 = [](){ return -1; };
    defaultdict<std::string, int> dd1(factory1, {{"a", 1}});
    dd1["b"];

    defaultdict<std::string, int> dd2(factory2, {{"x", 10}});
    dd2["y"];

    ASSERT_EQ(dd1.get_default_factory()(), 0);
    ASSERT_EQ(dd2.get_default_factory()(), -1);

    dd1.swap(dd2);

    ASSERT_EQ(dd1.size(), 2);
    ASSERT_TRUE(dd1.contains("x"));
    ASSERT_TRUE(dd1.contains("y"));
    ASSERT_EQ(dd1.at("x"), 10);
    ASSERT_EQ(dd1.at("y"), -1);
    ASSERT_EQ(dd1["new_in_dd1"], -1);

    ASSERT_EQ(dd2.size(), 2);
    ASSERT_TRUE(dd2.contains("a"));
    ASSERT_TRUE(dd2.contains("b"));
    ASSERT_EQ(dd2.at("a"), 1);
    ASSERT_EQ(dd2.at("b"), 0);
    ASSERT_EQ(dd2["new_in_dd2"], 0);
}

// Test Lookup (count, find, contains)
TEST_F(DefaultDictTest, Count) {
    defaultdict<std::string, int> dd(zero_factory<int>(), {{"a",1}});
    ASSERT_EQ(dd.count("a"), 1);
    ASSERT_EQ(dd.count("b"), 0);
    dd["b"];
    ASSERT_EQ(dd.count("b"), 1);
    ASSERT_EQ(dd.size(), 2);
}

TEST_F(DefaultDictTest, Find) {
    defaultdict<std::string, int> dd(zero_factory<int>(), {{"a",1}});
    auto it = dd.find("a");
    ASSERT_NE(it, dd.end());
    ASSERT_EQ(it->second, 1);

    auto it_non = dd.find("b");
    ASSERT_EQ(it_non, dd.end());
    ASSERT_EQ(dd.size(), 1);
}

TEST_F(DefaultDictTest, Contains) {
    defaultdict<std::string, int> dd(zero_factory<int>());
    dd["a"] = 1;
    ASSERT_TRUE(dd.contains("a"));
    ASSERT_FALSE(dd.contains("b"));
    ASSERT_EQ(dd.size(), 1);
}

// Test Iterators
TEST_F(DefaultDictTest, Iterators) {
    defaultdict<std::string, int> dd(zero_factory<int>(), {{"a",1}, {"b",2}, {"c",3}});
    int sum_val = 0;
    size_t count = 0;
    for(const auto& pair_val : dd) {
        sum_val += pair_val.second;
        count++;
    }
    ASSERT_EQ(count, 3);
    ASSERT_EQ(sum_val, 1+2+3);

    auto it_b = dd.find("b");
    ASSERT_NE(it_b, dd.end());
    it_b->second = 20;
    ASSERT_EQ(dd.at("b"), 20);
}

// Test Comparison Operators
TEST_F(DefaultDictTest, Equality) {
    defaultdict<std::string, int> dd1(zero_factory<int>(), {{"a",1}, {"b",2}});
    defaultdict<std::string, int> dd2(zero_factory<int>(), {{"b",2}, {"a",1}});
    defaultdict<std::string, int> dd3(zero_factory<int>(), {{"a",1}, {"c",3}});
    defaultdict<std::string, int> dd4(zero_factory<int>(), {{"a",1}, {"b",99}});

    ASSERT_TRUE(dd1 == dd2);
    ASSERT_FALSE(dd1 == dd3);
    ASSERT_FALSE(dd1 == dd4);
    ASSERT_TRUE(dd1 != dd3);

    dd1["z"] = 0;
    ASSERT_FALSE(dd1 == dd2); // dd1 has z, dd2 does not
    dd2["z"];
    ASSERT_TRUE(dd1 == dd2);
}

// Test Default Factory Management
TEST_F(DefaultDictTest, GetAndSetDefaultFactory) {
    defaultdict<std::string, int> dd(zero_factory<int>());
    ASSERT_EQ(dd["test1"], 0);

    dd.set_default_factory([](){ return 100; });
    ASSERT_EQ(dd.get_default_factory()(), 100);

    ASSERT_EQ(dd["test2"], 100);
    ASSERT_EQ(dd.at("test1"), 0);
    ASSERT_EQ(dd.size(), 2);
}

// Test 'extract' method
TEST_F(DefaultDictTest, ExtractExisting) {
    defaultdict<std::string, int> dd(zero_factory<int>(), {{"a", 10}, {"b", 20}});
    ASSERT_EQ(dd.size(), 2);
    ASSERT_TRUE(dd.contains("a"));

    auto node_handle = dd.extract("a");
    ASSERT_TRUE(static_cast<bool>(node_handle));
    ASSERT_FALSE(node_handle.empty());
    ASSERT_EQ(node_handle.key(), "a");
    ASSERT_EQ(node_handle.mapped(), 10);

    ASSERT_EQ(dd.size(), 1);
    ASSERT_FALSE(dd.contains("a"));
    ASSERT_THROW(dd.at("a"), std::out_of_range);
    ASSERT_EQ(dd["a"], 0);
    ASSERT_EQ(dd.size(), 2);
}

TEST_F(DefaultDictTest, ExtractNonExisting) {
    defaultdict<std::string, int> dd(zero_factory<int>(), {{"b", 20}});
    ASSERT_EQ(dd.size(), 1);

    auto node_handle = dd.extract("nonexistent");
    ASSERT_FALSE(static_cast<bool>(node_handle));
    ASSERT_TRUE(node_handle.empty());

    ASSERT_EQ(dd.size(), 1);
    ASSERT_FALSE(dd.contains("nonexistent"));
}

TEST_F(DefaultDictTest, ExtractDoesNotTriggerDefaultFactory) {
    defaultdict<std::string, int> dd(zero_factory<int>());
    ASSERT_EQ(dd.size(), 0);

    auto node_handle = dd.extract("nonexistent");
    ASSERT_FALSE(static_cast<bool>(node_handle));
    ASSERT_EQ(dd.size(), 0);
}

// Test 'merge' method
TEST_F(DefaultDictTest, MergeFromOtherDefaultDict) {
    defaultdict<std::string, int> dd1(zero_factory<int>(), {{"a", 1}, {"b", 2}});
    defaultdict<std::string, int> dd2(zero_factory<int>(), {{"b", 30}, {"c", 4}});

    dd1.merge(dd2);

    ASSERT_EQ(dd1.size(), 3);
    ASSERT_EQ(dd1.at("a"), 1);
    ASSERT_EQ(dd1.at("b"), 2);
    ASSERT_EQ(dd1.at("c"), 4);
    ASSERT_EQ(dd1["d"], 0);
    ASSERT_EQ(dd1.size(), 4);

    ASSERT_EQ(dd2.size(), 1);
    ASSERT_TRUE(dd2.contains("b"));
    ASSERT_EQ(dd2.at("b"), 30);
    ASSERT_FALSE(dd2.contains("c"));
}

TEST_F(DefaultDictTest, MergeFromStdUnorderedMap) {
    defaultdict<std::string, int> dd1(zero_factory<int>(), {{"a", 1}, {"b", 2}});
    std::unordered_map<std::string, int> umap = {{"b", 30}, {"c", 4}};

    dd1.merge(umap);

    ASSERT_EQ(dd1.size(), 3);
    ASSERT_EQ(dd1.at("a"), 1);
    ASSERT_EQ(dd1.at("b"), 2);
    ASSERT_EQ(dd1.at("c"), 4);

    ASSERT_EQ(umap.size(), 1);
    ASSERT_TRUE(umap.count("b"));
    ASSERT_EQ(umap.at("b"), 30);
}

TEST_F(DefaultDictTest, MergeEmptySource) {
    defaultdict<std::string, int> dd1(zero_factory<int>(), {{"a", 1}});
    defaultdict<std::string, int> dd_empty_dd(zero_factory<int>());
    std::unordered_map<std::string, int> umap_empty;

    size_t original_size = dd1.size();
    dd1.merge(dd_empty_dd);
    ASSERT_EQ(dd1.size(), original_size);
    ASSERT_TRUE(dd_empty_dd.empty());

    dd1.merge(umap_empty);
    ASSERT_EQ(dd1.size(), original_size);
    ASSERT_TRUE(umap_empty.empty());
}

TEST_F(DefaultDictTest, MergeIntoEmpty) {
    defaultdict<std::string, int> dd_empty(zero_factory<int>());
    defaultdict<std::string, int> dd_source(zero_factory<int>(), {{"a", 1}, {"b", 2}});
    size_t source_original_size = dd_source.size();

    dd_empty.merge(dd_source);
    ASSERT_EQ(dd_empty.size(), source_original_size);
    ASSERT_EQ(dd_empty.at("a"), 1);
    ASSERT_EQ(dd_empty.at("b"), 2);
    ASSERT_TRUE(dd_source.empty());
}

TEST_F(DefaultDictTest, MergeMaintainsOwnFactory) {
    defaultdict<std::string, int> dd1([](){ return 111; }, {{"a", 1}});
    defaultdict<std::string, int> dd2([](){ return 222; }, {{"b", 2}});

    dd1.merge(dd2);
    ASSERT_EQ(dd1.at("a"), 1);
    ASSERT_EQ(dd1.at("b"), 2);
    ASSERT_EQ(dd1["new_key_in_dd1"], 111);

    ASSERT_FALSE(dd2.contains("b"));
    ASSERT_TRUE(dd2.empty());
    ASSERT_EQ(dd2["new_key_in_dd2"], 222);
}

TEST_F(DefaultDictTest, FactoryWithReferenceCapture) {
    int default_val = 50;
    auto factory_ref_capture = [&default_val]() { return default_val; };
    defaultdict<std::string, int> dd(factory_ref_capture);

    ASSERT_EQ(dd["key1"], 50);
    default_val = 60;
    ASSERT_EQ(dd["key2"], 60);

    defaultdict<std::string, int> dd_copy(dd);
    default_val = 70;
    ASSERT_EQ(dd_copy["key3"], 70);
    ASSERT_EQ(dd["key_after_copy_check"], 70);

    int new_default_val = 80;
    dd.set_default_factory([&new_default_val](){ return new_default_val; });
    ASSERT_EQ(dd["key4_new_factory"], 80);
    new_default_val = 90;
    ASSERT_EQ(dd["key5_new_factory_updated"], 90);

    default_val = 75;
    ASSERT_EQ(dd_copy["key6_copy_old_factory"], 75);
}


struct MovableOnlyFactory {
    int val = 0;
    MovableOnlyFactory(int v = 0) : val(v) {}
    MovableOnlyFactory(const MovableOnlyFactory&) = delete;
    MovableOnlyFactory& operator=(const MovableOnlyFactory&) = delete;
    MovableOnlyFactory(MovableOnlyFactory&& other) noexcept : val(other.val) { other.val = -1; }
    MovableOnlyFactory& operator=(MovableOnlyFactory&& other) noexcept {
        if (this != &other) {
            val = other.val;
            other.val = -1;
        }
        return *this;
    }
    int operator()() const { return val; }
};

TEST_F(DefaultDictTest, MovableOnlyFactoryType) {
    defaultdict<std::string, int, MovableOnlyFactory> dd(MovableOnlyFactory(10));
    ASSERT_EQ(dd["test"], 10);

    defaultdict<std::string, int, MovableOnlyFactory> dd_moved(std::move(dd));
    ASSERT_EQ(dd_moved["test2"], 10);

    dd_moved.set_default_factory(MovableOnlyFactory(20));
    ASSERT_EQ(dd_moved["test3"], 20);
}

TEST_F(DefaultDictTest, DefaultConstructionWithDefaultFactoryHelper) {
    defaultdict<std::string, MyStruct> dd(default_factory<MyStruct>());
    ASSERT_EQ(dd["test"].id, 0);
    ASSERT_EQ(dd["test"].data, "default_constructed");
}

TEST_F(DefaultDictTest, StringFactoryHelper) {
    defaultdict<int, std::string> dd(string_factory());
    ASSERT_EQ(dd[1], "");
}

TEST_F(DefaultDictTest, ZeroFactoryHelper) {
    defaultdict<std::string, double> dd(zero_factory<double>());
    ASSERT_DOUBLE_EQ(dd["val"], 0.0);
}
