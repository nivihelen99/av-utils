#include "variant_vector.h"

// ============================================================================
// USAGE EXAMPLE AND BENCHMARKING
// ============================================================================

#include <chrono>
#include <iostream>
#include <random>

// Test data types
struct SmallData { 
    int x; 
    SmallData() = default;
    SmallData(int x_) : x(x_) {}
};

struct MediumData { 
    int x, y; 
    double z; 
    MediumData() = default;
    MediumData(int x_, int y_, double z_) : x(x_), y(y_), z(z_) {}
};

struct LargeData { 
    std::array<double, 16> data;
    std::string name;
    LargeData() = default;
    LargeData(const std::string& name_) : name(name_) {
        std::fill(data.begin(), data.end(), 0.0);
    }
};

void benchmark_memory_usage() {
    const size_t N = 100000;
    
    // Traditional std::vector<std::variant>
    std::vector<std::variant<SmallData, MediumData, LargeData>> traditional;
    traditional.reserve(N);
    
    // Our optimized static version
    static_variant_vector<SmallData, MediumData, LargeData> optimized_static;
    optimized_static.reserve(N);
    
    // Our optimized dynamic version
    dynamic_variant_vector optimized_dynamic;
    optimized_dynamic.reserve(N);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 2);
    
    // Fill containers
    for (size_t i = 0; i < N; ++i) {
        int type = dis(gen);
        switch (type) {
            case 0:
                traditional.emplace_back(SmallData{static_cast<int>(i)});
                optimized_static.emplace_back<SmallData>(static_cast<int>(i));
                optimized_dynamic.emplace_back<SmallData>(static_cast<int>(i));
                break;
            case 1:
                traditional.emplace_back(MediumData{static_cast<int>(i), static_cast<int>(i*2), static_cast<double>(i)});
                optimized_static.emplace_back<MediumData>(static_cast<int>(i), static_cast<int>(i*2), static_cast<double>(i));
                optimized_dynamic.emplace_back<MediumData>(static_cast<int>(i), static_cast<int>(i*2), static_cast<double>(i));
                break;
            case 2: {
                LargeData ld("item_" + std::to_string(i));
                std::fill(ld.data.begin(), ld.data.end(), static_cast<double>(i));
                traditional.emplace_back(std::move(ld));
                
                LargeData ld2("item_" + std::to_string(i));
                std::fill(ld2.data.begin(), ld2.data.end(), static_cast<double>(i));
                optimized_static.emplace_back<LargeData>(std::move(ld2));
                
                LargeData ld3("item_" + std::to_string(i));
                std::fill(ld3.data.begin(), ld3.data.end(), static_cast<double>(i));
                optimized_dynamic.emplace_back<LargeData>(std::move(ld3));
                break;
            }
        }
    }
    
    std::cout << "Memory Usage Comparison (for " << N << " elements):\n";
    std::cout << "Traditional vector<variant>: " << traditional.capacity() * sizeof(std::variant<SmallData, MediumData, LargeData>) << " bytes\n";
    std::cout << "Optimized static SoA:       " << optimized_static.memory_usage() << " bytes\n";
    std::cout << "Optimized dynamic SoA:      " << optimized_dynamic.memory_usage() << " bytes\n";
    
    // Performance test - type-specific operations
    auto start = std::chrono::high_resolution_clock::now();
    
    // Access all SmallData elements from optimized version (cache-friendly)
    const auto& small_vec = optimized_static.get_type_vector<SmallData>();
    long long sum = 0;
    for (const auto& item : small_vec) {
        sum += item.x;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto optimized_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    start = std::chrono::high_resolution_clock::now();
    
    // Access all SmallData elements from traditional version (cache-unfriendly)
    sum = 0;
    for (const auto& variant : traditional) {
        if (std::holds_alternative<SmallData>(variant)) {
            sum += std::get<SmallData>(variant).x;
        }
    }
    
    end = std::chrono::high_resolution_clock::now();
    auto traditional_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "\nPerformance Comparison (sum of SmallData elements):\n";
    std::cout << "Traditional approach: " << traditional_time.count() << " microseconds\n";
    std::cout << "Optimized SoA approach: " << optimized_time.count() << " microseconds\n";
    std::cout << "Speedup: " << static_cast<double>(traditional_time.count()) / optimized_time.count() << "x\n";
}

void demonstrate_new_features() {
    std::cout << "\n--- Demonstrating new features ---\n";
    static_variant_vector<SmallData, MediumData, LargeData> s_vec;
    s_vec.emplace_back<SmallData>(101);
    s_vec.emplace_back<MediumData>(102, 103, 104.0);
    std::cout << "Static vec size before pop_back: " << s_vec.size() << '\n'; // Expected: 2
    s_vec.pop_back(); // Pops MediumData
    std::cout << "Static vec size after pop_back: " << s_vec.size() << '\n';  // Expected: 1
    if (s_vec.size() == 1) {
        auto val = s_vec[0];
        if(std::holds_alternative<SmallData>(val)) {
             std::cout << "Remaining element is SmallData with value: " << std::get<SmallData>(val).x << '\n';
        }
    }
    s_vec.clear();
    std::cout << "Static vec size after clear: " << s_vec.size() << '\n';    // Expected: 0

    dynamic_variant_vector d_vec;
    d_vec.emplace_back<SmallData>(201);
    d_vec.emplace_back<LargeData>("example_large");
    std::cout << "Dynamic vec size before pop_back: " << d_vec.size() << '\n'; // Expected: 2
    d_vec.pop_back(); // Pops LargeData
    std::cout << "Dynamic vec size after pop_back: " << d_vec.size() << '\n';  // Expected: 1
    if (d_vec.size() == 1) {
        SmallData& val = d_vec.get_typed<SmallData>(0);
        std::cout << "Remaining element in dynamic_vec is SmallData with value: " << val.x << '\n';
        // Demonstrate std::any at()
        std::any any_val = d_vec.at(0);
        if (any_val.type() == typeid(SmallData)) {
            std::cout << "Dynamic vec at(0) via std::any has SmallData with value: " << std::any_cast<SmallData>(any_val).x << '\n';
        }
    }
    d_vec.clear();
    std::cout << "Dynamic vec size after clear: " << d_vec.size() << '\n';    // Expected: 0
}


int main() {
    benchmark_memory_usage();
    demonstrate_new_features(); // Call the new function
    return 0;
}

