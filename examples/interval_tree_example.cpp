#include "interval_tree.h"
#include <iostream>
#include <string>
#include <cassert>

using namespace interval_tree;

void basic_example() {
    std::cout << "=== Basic Example ===\n";
    
    IntervalTree<std::string> tree;
    
    // Insert intervals
    tree.insert(10, 20, "A");
    tree.insert(15, 25, "B");
    tree.insert(30, 40, "C");
    tree.insert(5, 15, "D");
    
    std::cout << "Tree size: " << tree.size() << "\n";
    
    // Query point
    auto overlaps = tree.query(18);
    std::cout << "Intervals overlapping point 18: ";
    for (const auto& iv : overlaps) {
        std::cout << "[" << iv.start << "," << iv.end << "):" << iv.value << " ";
    }
    std::cout << "\n";
    
    // Query range
    auto range_overlaps = tree.query(12, 17);
    std::cout << "Intervals overlapping range [12,17): ";
    for (const auto& iv : range_overlaps) {
        std::cout << "[" << iv.start << "," << iv.end << "):" << iv.value << " ";
    }
    std::cout << "\n";
    
    // Remove an interval
    tree.remove(15, 25, "B");
    std::cout << "After removing [15,25):B, size: " << tree.size() << "\n";
    
    // Query again
    overlaps = tree.query(18);
    std::cout << "Intervals overlapping point 18 after removal: ";
    for (const auto& iv : overlaps) {
        std::cout << "[" << iv.start << "," << iv.end << "):" << iv.value << " ";
    }
    std::cout << "\n\n";
}

void scheduling_example() {
    std::cout << "=== Scheduling Example ===\n";
    
    struct Meeting {
        std::string title;
        std::string room;
        
        Meeting(std::string t, std::string r) : title(std::move(t)), room(std::move(r)) {}
        
        bool operator==(const Meeting& other) const {
            return title == other.title && room == other.room;
        }
    };
    
    IntervalTree<Meeting> schedule;
    
    // Add meetings (times in minutes from midnight)
    schedule.insert(540, 600, Meeting("Team Standup", "Room A"));      // 9:00-10:00
    schedule.insert(570, 630, Meeting("Client Call", "Room B"));       // 9:30-10:30
    schedule.insert(720, 780, Meeting("Design Review", "Room A"));     // 12:00-13:00
    schedule.insert(840, 900, Meeting("Sprint Planning", "Room C"));   // 14:00-15:00
    
    // Check for conflicts at 9:45 (585 minutes)
    auto conflicts = schedule.query(585);
    std::cout << "Meetings at 9:45 AM: ";
    for (const auto& meeting : conflicts) {
        int start_hour = meeting.start / 60;
        int start_min = meeting.start % 60;
        int end_hour = meeting.end / 60;
        int end_min = meeting.end % 60;
        
        std::cout << meeting.value.title << " (" << start_hour << ":" 
                  << (start_min < 10 ? "0" : "") << start_min << "-"
                  << end_hour << ":" << (end_min < 10 ? "0" : "") << end_min
                  << " in " << meeting.value.room << ") ";
    }
    std::cout << "\n";
    
    // Find meetings in lunch time range (12:00-13:30)
    auto lunch_conflicts = schedule.query(720, 810);
    std::cout << "Meetings during lunch (12:00-13:30): ";
    for (const auto& meeting : lunch_conflicts) {
        std::cout << meeting.value.title << " ";
    }
    std::cout << "\n\n";
}

void memory_regions_example() {
    std::cout << "=== Memory Regions Example ===\n";
    
    struct MemoryInfo {
        std::string type;
        bool writable;
        
        MemoryInfo(std::string t, bool w) : type(std::move(t)), writable(w) {}
        
        bool operator==(const MemoryInfo& other) const {
            return type == other.type && writable == other.writable;
        }
    };
    
    IntervalTree<MemoryInfo> memory_map;
    
    // Map memory regions (addresses in hex, but using decimal for simplicity)
    memory_map.insert(0x1000, 0x2000, MemoryInfo("Code", false));
    memory_map.insert(0x2000, 0x3000, MemoryInfo("Data", true));
    memory_map.insert(0x3000, 0x4000, MemoryInfo("Heap", true));
    memory_map.insert(0x8000, 0x9000, MemoryInfo("Stack", true));
    
    // Check what's at address 0x2500
    auto regions = memory_map.query(0x2500);
    std::cout << "Memory region at address 0x2500: ";
    for (const auto& region : regions) {
        std::cout << region.value.type << " (writable: " << region.value.writable << ") ";
    }
    std::cout << "\n";
    
    // Check overlapping regions in range 0x1800-0x2800
    auto overlapping = memory_map.query(0x1800, 0x2800);
    std::cout << "Memory regions overlapping 0x1800-0x2800: ";
    for (const auto& region : overlapping) {
        std::cout << region.value.type << " ";
    }
    std::cout << "\n\n";
}

void performance_test() {
    std::cout << "=== Performance Test ===\n";
    
    IntervalTree<int> tree;
    
    // Insert many intervals
    const int N = 10000;
    for (int i = 0; i < N; ++i) {
        tree.insert(i * 10, i * 10 + 50, i);
    }
    
    std::cout << "Inserted " << N << " intervals\n";
    std::cout << "Tree size: " << tree.size() << "\n";
    
    // Query a point that should hit many intervals
    auto overlaps = tree.query(250000);
    std::cout << "Intervals overlapping point 250000: " << overlaps.size() << "\n";
    
    // Query a range
    auto range_overlaps = tree.query(100000, 100100);
    std::cout << "Intervals overlapping range [100000,100100): " << range_overlaps.size() << "\n";
    
    std::cout << "\n";
}

void test_edge_cases() {
    std::cout << "=== Edge Cases Test ===\n";
    
    IntervalTree<std::string> tree;
    
    // Test empty tree
    assert(tree.empty());
    assert(tree.size() == 0);
    auto empty_result = tree.query(10);
    assert(empty_result.empty());
    
    // Test single interval
    tree.insert(10, 20, "single");
    assert(tree.size() == 1);
    assert(!tree.empty());
    
    auto single_result = tree.query(15);
    assert(single_result.size() == 1);
    assert(single_result[0].value == "single");
    
    // Test point at boundaries
    auto boundary1 = tree.query(10);  // Should include
    assert(boundary1.size() == 1);
    
    auto boundary2 = tree.query(20);  // Should not include (exclusive end)
    assert(boundary2.empty());
    
    auto boundary3 = tree.query(9);   // Should not include
    assert(boundary3.empty());
    
    // Test identical intervals with different values
    tree.insert(10, 20, "duplicate1");
    tree.insert(10, 20, "duplicate2");
    assert(tree.size() == 3);
    
    auto duplicates = tree.query(15);
    assert(duplicates.size() == 3);
    
    // Remove specific duplicate
    tree.remove(10, 20, "duplicate1");
    assert(tree.size() == 2);
    
    duplicates = tree.query(15);
    assert(duplicates.size() == 2);
    
    // Clear tree
    tree.clear();
    assert(tree.empty());
    assert(tree.size() == 0);
    
    std::cout << "All edge case tests passed!\n\n";
}

int main() {
    basic_example();
    scheduling_example();
    memory_regions_example();
    performance_test();
    test_edge_cases();
    
    std::cout << "All examples and tests completed successfully!\n";
    return 0;
}
