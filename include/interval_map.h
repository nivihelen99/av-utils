#pragma once
#include <vector>
#include <tuple>
#include <algorithm>
#include <stdexcept>
#include <map>
#include <set>

// Approach 1: Simple vector-based implementation
// Good for small to medium datasets, excellent cache locality
template <typename T>
class IntervalMapVector {
private:
    struct Interval {
        int start, end;
        T value;
        
        Interval(int s, int e, const T& v) : start(s), end(e), value(v) {}
        
        bool operator<(const Interval& other) const {
            if (start != other.start) return start < other.start;
            return end < other.end;
        }
        
        bool overlaps_point(int point) const {
            return point >= start && point < end;
        }
        
        bool overlaps_range(int qstart, int qend) const {
            return start < qend && qstart < end;
        }
    };
    
    std::vector<Interval> intervals;
    bool sorted = true;
    
    void ensure_sorted() {
        if (!sorted) {
            std::sort(intervals.begin(), intervals.end());
            sorted = true;
        }
    }

public:
    void insert(int start, int end, const T& value) {
        if (start >= end) {
            throw std::invalid_argument("Invalid interval: start must be less than end");
        }
        
        // Check for duplicates
        for (const auto& interval : intervals) {
            if (interval.start == start && interval.end == end) {
                throw std::invalid_argument("Duplicate interval not allowed");
            }
        }
        
        intervals.emplace_back(start, end, value);
        sorted = false;
    }
    
    bool remove(int start, int end) {
        auto it = std::find_if(intervals.begin(), intervals.end(),
            [start, end](const Interval& interval) {
                return interval.start == start && interval.end == end;
            });
        
        if (it != intervals.end()) {
            intervals.erase(it);
            return true;
        }
        return false;
    }
    
    std::vector<T> query(int point) const {
        std::vector<T> result;
        for (const auto& interval : intervals) {
            if (interval.overlaps_point(point)) {
                result.push_back(interval.value);
            }
        }
        return result;
    }
    
    std::vector<T> query_range(int qstart, int qend) const {
        if (qstart >= qend) return {};
        
        std::vector<T> result;
        for (const auto& interval : intervals) {
            if (interval.overlaps_range(qstart, qend)) {
                result.push_back(interval.value);
            }
        }
        return result;
    }
    
    std::vector<std::tuple<int, int, T>> all_intervals() const {
        std::vector<std::tuple<int, int, T>> result;
        for (const auto& interval : intervals) {
            result.emplace_back(interval.start, interval.end, interval.value);
        }
        return result;
    }
    
    bool empty() const { return intervals.empty(); }
    size_t size() const { return intervals.size(); }
};

// Approach 2: Map-based implementation with better query performance
// Uses std::map for O(log n) insertions and smarter queries
template <typename T>
class IntervalMapSorted {
private:
    // Map from start point to (end, value) pairs
    std::map<int, std::vector<std::pair<int, T>>> start_map;
    
public:
    void insert(int start, int end, const T& value) {
        if (start >= end) {
            throw std::invalid_argument("Invalid interval: start must be less than end");
        }
        
        // Check for duplicates in this start group
        auto& intervals = start_map[start];
        for (const auto& [existing_end, existing_value] : intervals) {
            if (existing_end == end) {
                throw std::invalid_argument("Duplicate interval not allowed");
            }
        }
        
        intervals.emplace_back(end, value);
    }
    
    bool remove(int start, int end) {
        auto it = start_map.find(start);
        if (it == start_map.end()) return false;
        
        auto& intervals = it->second;
        auto interval_it = std::find_if(intervals.begin(), intervals.end(),
            [end](const std::pair<int, T>& p) { return p.first == end; });
        
        if (interval_it != intervals.end()) {
            intervals.erase(interval_it);
            if (intervals.empty()) {
                start_map.erase(it);
            }
            return true;
        }
        return false;
    }
    
    std::vector<T> query(int point) const {
        std::vector<T> result;
        
        // Find all intervals that could contain this point
        // We need to check all starts <= point
        auto upper = start_map.upper_bound(point);
        for (auto it = start_map.begin(); it != upper; ++it) {
            for (const auto& [end, value] : it->second) {
                if (point < end) {  // point >= start (guaranteed by iteration)
                    result.push_back(value);
                }
            }
        }
        return result;
    }
    
    std::vector<T> query_range(int qstart, int qend) const {
        if (qstart >= qend) return {};
        
        std::vector<T> result;
        
        // Check all intervals for overlap
        for (const auto& [start, intervals] : start_map) {
            for (const auto& [end, value] : intervals) {
                if (start < qend && qstart < end) {
                    result.push_back(value);
                }
            }
        }
        return result;
    }
    
    std::vector<std::tuple<int, int, T>> all_intervals() const {
        std::vector<std::tuple<int, int, T>> result;
        for (const auto& [start, intervals] : start_map) {
            for (const auto& [end, value] : intervals) {
                result.emplace_back(start, end, value);
            }
        }
        return result;
    }
    
    bool empty() const { return start_map.empty(); }
    
    size_t size() const {
        size_t count = 0;
        for (const auto& [start, intervals] : start_map) {
            count += intervals.size();
        }
        return count;
    }
};

// Approach 3: Segment tree approach using STL (for very large datasets)
template <typename T>
class IntervalMapSegment {
private:
    struct Event {
        int pos;
        int type; // 1 for start, -1 for end
        T value;
        int interval_id;
        
        bool operator<(const Event& other) const {
            if (pos != other.pos) return pos < other.pos;
            return type > other.type; // Process starts before ends
        }
    };
    
    std::vector<std::tuple<int, int, T>> intervals;
    int next_id = 0;

public:
    void insert(int start, int end, const T& value) {
        if (start >= end) {
            throw std::invalid_argument("Invalid interval: start must be less than end");
        }
        
        // Check for duplicates
        for (const auto& [s, e, v] : intervals) {
            if (s == start && e == end) {
                throw std::invalid_argument("Duplicate interval not allowed");
            }
        }
        
        intervals.emplace_back(start, end, value);
    }
    
    bool remove(int start, int end) {
        auto it = std::find_if(intervals.begin(), intervals.end(),
            [start, end](const std::tuple<int, int, T>& interval) {
                return std::get<0>(interval) == start && std::get<1>(interval) == end;
            });
        
        if (it != intervals.end()) {
            intervals.erase(it);
            return true;
        }
        return false;
    }
    
    std::vector<T> query(int point) const {
        std::vector<T> result;
        for (const auto& [start, end, value] : intervals) {
            if (point >= start && point < end) {
                result.push_back(value);
            }
        }
        return result;
    }
    
    std::vector<T> query_range(int qstart, int qend) const {
        if (qstart >= qend) return {};
        
        std::vector<T> result;
        for (const auto& [start, end, value] : intervals) {
            if (start < qend && qstart < end) {
                result.push_back(value);
            }
        }
        return result;
    }
    
    std::vector<std::tuple<int, int, T>> all_intervals() const {
        return intervals;
    }
    
    bool empty() const { return intervals.empty(); }
    size_t size() const { return intervals.size(); }
};

// Default to the map-based approach for good balance of performance and simplicity
template <typename T>
using IntervalMap = IntervalMapSorted<T>;

// Example usage and test cases
#ifdef INTERVAL_MAP_TEST
#include <iostream>
#include <cassert>
#include <string>
#include <chrono>

void test_basic_operations() {
    std::cout << "Testing basic operations...\n";
    
    IntervalMap<std::string> map;
    
    // Test insertion
    map.insert(10, 20, "Rule1");
    map.insert(15, 25, "Rule2");
    map.insert(30, 40, "Rule3");
    
    // Test point queries
    auto result1 = map.query(12);
    assert(result1.size() == 1 && result1[0] == "Rule1");
    
    auto result2 = map.query(18);
    assert(result2.size() == 2);
    
    auto result3 = map.query(35);
    assert(result3.size() == 1 && result3[0] == "Rule3");
    
    auto result4 = map.query(5);
    assert(result4.empty());
    
    std::cout << "Basic operations test passed!\n";
}

void test_range_queries() {
    std::cout << "Testing range queries...\n";
    
    IntervalMap<int> map;
    map.insert(10, 20, 1);
    map.insert(15, 25, 2);
    map.insert(30, 40, 3);
    map.insert(35, 45, 4);
    
    // Query range [12, 18) - should overlap with intervals 1 and 2
    auto result = map.query_range(12, 18);
    assert(result.size() == 2);
    
    // Query range [25, 30) - should have no overlaps
    result = map.query_range(25, 30);
    assert(result.empty());
    
    // Query range [32, 42) - should overlap with intervals 3 and 4
    result = map.query_range(32, 42);
    assert(result.size() == 2);
    
    std::cout << "Range queries test passed!\n";
}

void test_removal() {
    std::cout << "Testing removal...\n";
    
    IntervalMap<std::string> map;
    map.insert(10, 20, "A");
    map.insert(15, 25, "B");
    map.insert(30, 40, "C");
    
    assert(map.size() == 3);
    
    // Remove existing interval
    bool removed = map.remove(15, 25);
    assert(removed);
    assert(map.size() == 2);
    
    // Try to remove non-existing interval
    removed = map.remove(100, 200);
    assert(!removed);
    assert(map.size() == 2);
    
    // Verify the removal worked
    auto result = map.query(18);
    assert(result.size() == 1 && result[0] == "A");
    
    std::cout << "Removal test passed!\n";
}

void test_edge_cases() {
    std::cout << "Testing edge cases...\n";
    
    IntervalMap<int> map;
    
    // Test invalid intervals
    try {
        map.insert(20, 10, 1);  // start >= end
        assert(false);  // Should not reach here
    } catch (const std::invalid_argument&) {
        // Expected
    }
    
    try {
        map.insert(10, 10, 1);  // start == end
        assert(false);  // Should not reach here
    } catch (const std::invalid_argument&) {
        // Expected
    }
    
    // Test duplicate intervals
    map.insert(10, 20, 1);
    try {
        map.insert(10, 20, 2);  // Exact duplicate
        assert(false);  // Should not reach here
    } catch (const std::invalid_argument&) {
        // Expected
    }
    
    // Test boundary conditions
    map.insert(0, 10, 2);
    map.insert(20, 30, 3);
    
    auto result = map.query(10);  // Boundary point
    assert(result.size() == 1 && result[0] == 1);
    
    result = map.query(20);  // Another boundary
    assert(result.size() == 1 && result[0] == 3);
    
    std::cout << "Edge cases test passed!\n";
}

void benchmark_implementations() {
    std::cout << "Benchmarking different implementations...\n";
    
    const int N = 10000;
    const int QUERIES = 1000;
    
    // Test data
    std::vector<std::tuple<int, int, int>> test_intervals;
    for (int i = 0; i < N; ++i) {
        int start = rand() % 100000;
        int end = start + (rand() % 1000) + 1;
        test_intervals.emplace_back(start, end, i);
    }
    
    // Benchmark Vector implementation
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        IntervalMapVector<int> map;
        for (const auto& [s, e, v] : test_intervals) {
            map.insert(s, e, v);
        }
        
        for (int i = 0; i < QUERIES; ++i) {
            map.query(rand() % 100000);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        std::cout << "Vector implementation: " << duration.count() << "ms\n";
    }
    
    // Benchmark Map implementation
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        IntervalMapSorted<int> map;
        for (const auto& [s, e, v] : test_intervals) {
            map.insert(s, e, v);
        }
        
        for (int i = 0; i < QUERIES; ++i) {
            map.query(rand() % 100000);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        std::cout << "Map implementation: " << duration.count() << "ms\n";
    }
}

int main() {
    test_basic_operations();
    test_range_queries();
    test_removal();
    test_edge_cases();
    benchmark_implementations();
    
    std::cout << "All tests passed!\n";
    return 0;
}
#endif
