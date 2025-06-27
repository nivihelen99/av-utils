#include "cached_property.h" // Should be <cached_property.h> if installed, "..." for local build
#include <string>
#include <vector>
#include <iostream>
#include <cassert>
#include <stdexcept> // For std::invalid_argument

// Define the example namespace and its contents here
namespace cached_property_example {

class DataProcessor {
public:
    std::string name;
    std::vector<int> values;
    mutable int expensive_calc_count = 0;
    mutable int sum_calc_count = 0;
    mutable int size_calc_count = 0;


    DataProcessor(std::string n, std::vector<int> v) : name(std::move(n)), values(std::move(v)) {
        std::cout << "DataProcessor '" << name << "' created." << std::endl;
    }

    // Property computed by a lambda
    CachedProperty<DataProcessor, double> average{this, [](DataProcessor* self) {
        std::cout << "Computing average for '" << self->name << "'..." << std::endl;
        self->expensive_calc_count++;
        if (self->values.empty()) {
            return 0.0;
        }
        double sum = 0;
        for (int val : self->values) {
            sum += val;
        }
        return sum / self->values.size();
    }};

    // Property computed by a non-const member function
    CachedProperty<DataProcessor, int> total_sum{this, &DataProcessor::calculate_sum};

    // Property computed by a const member function
    CachedProperty<DataProcessor, size_t> data_size{this, &DataProcessor::get_data_size};

private:
    int calculate_sum() { // Non-const member function
        std::cout << "Computing total_sum for '" << this->name << "'..." << std::endl;
        this->sum_calc_count++;
        int sum = 0;
        for (int val : values) {
            sum += val;
        }
        return sum;
    }

    size_t get_data_size() const { // Const member function
        std::cout << "Computing data_size for '" << this->name << "'..." << std::endl;
        this->size_calc_count++;
        return values.size();
    }

public:
    void print_summary() const {
        std::cout << "Summary for " << name << ":" << std::endl;
        std::cout << "  Average: " << average.get() << std::endl;
        std::cout << "  Size: " << data_size << std::endl;
        // Accessing total_sum (non-const compute func) from const print_summary:
        // This is fine because CachedProperty::get() is const and owner_ptr_ is non-const.
        // The compute_func itself is called with a non-const `this`, but CachedProperty's const getter
        // allows it. If calculate_sum truly modified DataProcessor in an observable way (not just a counter),
        // it would be a logical const issue, but not a C++ language one here.
        std::cout << "  Total Sum (from const context): " << total_sum.get() << std::endl;
    }
};

void run_example() {
    std::cout << "--- CachedProperty Example ---" << std::endl;
    DataProcessor dp1("DP1", {1, 2, 3, 4, 5});
    DataProcessor dp2("DP2", {10, 20, 30});

    std::cout << "Accessing average for dp1 first time: " << dp1.average << std::endl;
    assert(dp1.expensive_calc_count == 1);
    std::cout << "Accessing average for dp1 second time: " << dp1.average << std::endl;
    assert(dp1.expensive_calc_count == 1);

    std::cout << "Accessing total_sum for dp1 first time: " << dp1.total_sum << std::endl;
    assert(dp1.sum_calc_count == 1);
    std::cout << "Accessing total_sum for dp1 second time: " << dp1.total_sum << std::endl;
    assert(dp1.sum_calc_count == 1);

    std::cout << "Accessing data_size for dp1 first time: " << dp1.data_size << std::endl;
    assert(dp1.size_calc_count == 1);
    std::cout << "Accessing data_size for dp1 second time: " << dp1.data_size << std::endl;
    assert(dp1.size_calc_count == 1);

    std::cout << "Accessing average for dp2: " << dp2.average << std::endl;
    assert(dp2.expensive_calc_count == 1);
    std::cout << "Accessing total_sum for dp2: " << dp2.total_sum << std::endl;
    assert(dp2.sum_calc_count == 1);
    std::cout << "Accessing data_size for dp2: " << dp2.data_size << std::endl;
    assert(dp2.size_calc_count == 1);

    dp1.print_summary();
    dp2.print_summary();

    std::cout << "Invalidating average for dp1..." << std::endl;
    dp1.average.invalidate();
    assert(!dp1.average.is_cached());
    std::cout << "Accessing average for dp1 after invalidation: " << dp1.average << std::endl;
    assert(dp1.expensive_calc_count == 2); // Recomputed

    // Using make_cached_property helper
    struct MySimpleStruct {
        int x = 5;
        int calc_count = 0;
        // Explicitly specify Owner and T for CachedProperty
        CachedProperty<MySimpleStruct, int> x_doubled;

        MySimpleStruct() : x_doubled(make_cached_property(this, [](MySimpleStruct* s){
            std::cout << "Calculating x_doubled for MySimpleStruct..." << std::endl;
            s->calc_count++;
            return s->x * 2;
        })) {}
    };
    MySimpleStruct ss;
    std::cout << "MySimpleStruct x_doubled: " << ss.x_doubled << std::endl;
    assert(ss.calc_count == 1);
    std::cout << "MySimpleStruct x_doubled again: " << ss.x_doubled << std::endl;
    assert(ss.calc_count == 1);

    struct MemberFuncStruct {
        int val = 10;
        mutable int process_value_count = 0;
        mutable int const_process_value_count = 0;

        CachedProperty<MemberFuncStruct, int> processed_val;
        CachedProperty<MemberFuncStruct, int> const_processed_val;

        MemberFuncStruct() :
            processed_val(make_cached_property(this, &MemberFuncStruct::process_value)),
            const_processed_val(make_cached_property(this, &MemberFuncStruct::const_process_value))
            {}

        int process_value() { // Can be non-const
            std::cout << "Calculating MemberFuncStruct::process_value..." << std::endl;
            process_value_count++;
            return val * 3;
        }
        int const_process_value() const { // Must be const if used with const Owner*
            std::cout << "Calculating MemberFuncStruct::const_process_value..." << std::endl;
            const_process_value_count++;
            return val * 4;
        }
    };
    MemberFuncStruct mfs;
    std::cout << "MemberFuncStruct processed_val: " << mfs.processed_val << std::endl;
    assert(mfs.process_value_count == 1);
    std::cout << "MemberFuncStruct processed_val again: " << mfs.processed_val << std::endl;
    assert(mfs.process_value_count == 1);

    std::cout << "MemberFuncStruct const_processed_val: " << mfs.const_processed_val << std::endl;
    assert(mfs.const_process_value_count == 1);
    std::cout << "MemberFuncStruct const_processed_val again: " << mfs.const_processed_val << std::endl;
    assert(mfs.const_process_value_count == 1);

    // Example with null owner pointer (should throw)
    try {
        CachedProperty<DataProcessor, int> null_owner_prop(nullptr, [](DataProcessor*){ return 0; });
        // The line above should throw. If not, the get() call will.
        // std::cout << "This should not print: " << null_owner_prop.get() << std::endl;
    } catch (const std::invalid_argument& e) {
        std::cout << "Caught expected exception for null owner: " << e.what() << std::endl;
    }

    // Example with null compute function (should throw)
    DataProcessor dp_temp("TEMP", {});
    try {
        CachedProperty<DataProcessor, int> null_func_prop(&dp_temp, nullptr);
        // The line above should throw. If not, the get() call will.
        // std::cout << "This should not print: " << null_func_prop.get() << std::endl;
    } catch (const std::invalid_argument& e) {
        std::cout << "Caught expected exception for null compute_func: " << e.what() << std::endl;
    }


    std::cout << "--- End CachedProperty Example ---" << std::endl;
}

} // namespace cached_property_example

int main() {
    cached_property_example::run_example();
    return 0;
}
