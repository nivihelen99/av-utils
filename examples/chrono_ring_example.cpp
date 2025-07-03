#include "chrono_ring.h" // Assuming this path will be correct via CMake include_directories
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <iomanip> // For std::setw

// Helper to print entries
template<typename T>
void print_entries(const std::vector<typename anomeda::ChronoRing<T>::Entry>& entries, const std::string& title) {
    std::cout << "\n--- " << title << " ---" << std::endl;
    if (entries.empty()) {
        std::cout << "(empty)" << std::endl;
        return;
    }
    for (const auto& entry : entries) {
        // Print timestamp relative to a chosen epoch for readability if possible, or as raw count
        auto time_since_epoch = entry.timestamp.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
        std::cout << "Value: " << std::setw(3) << entry.value
                  << ", Timestamp (ms since epoch): " << millis << std::endl;
    }
}

template<typename T>
void print_values(const std::vector<T>& values, const std::string& title) {
    std::cout << "\n--- " << title << " ---" << std::endl;
    if (values.empty()) {
        std::cout << "(empty)" << std::endl;
        return;
    }
    for (const auto& value : values) {
        std::cout << "Value: " << value << std::endl;
    }
}

int main() {
    using namespace std::chrono_literals;
    using Clock = std::chrono::steady_clock; // Same as default for ChronoRing

    std::cout << "ChronoRing Example\n";
    std::cout << "==================\n";

    anomeda::ChronoRing<int> ring(5);
    std::cout << "Created ChronoRing<int> with capacity 5.\n";
    std::cout << "Initial size: " << ring.size() << ", empty: " << std::boolalpha << ring.empty() << std::endl;

    // Push some values
    std::cout << "\nPushing values...\n";
    for (int i = 1; i <= 3; ++i) {
        ring.push(i * 10);
        std::cout << "Pushed " << i * 10 << ". Size: " << ring.size() << std::endl;
        std::this_thread::sleep_for(100ms);
    }

    auto entries_after_3 = ring.entries_between(Clock::time_point::min(), Clock::time_point::max());
    print_entries<int>(entries_after_3, "All entries after pushing 3 items");

    std::cout << "\nPushing more values to fill and wrap...\n";
    for (int i = 4; i <= 7; ++i) {
        ring.push(i * 10);
        std::cout << "Pushed " << i * 10 << ". Size: " << ring.size() << std::endl;
        std::this_thread::sleep_for(100ms); // ~700ms total from first push
    }

    auto entries_after_wrap = ring.entries_between(Clock::time_point::min(), Clock::time_point::max());
    print_entries<int>(entries_after_wrap, "All entries after pushing 7 items (capacity 5)");
    std::cout << "Note: Oldest values (10, 20) should be overwritten by (60, 70).\n";


    // Query recent items
    std::cout << "\nQuerying recent items (last 250ms)...\n";
    // Total time elapsed is roughly 7 * 100ms = 700ms.
    // So, items pushed in the last ~250ms should be 70, 60, maybe 50.
    auto recent_values = ring.recent(250ms);
    print_values<int>(recent_values, "Values in the last 250ms");


    // Expire older items
    std::cout << "\nExpiring items older than ~450ms from now...\n";
    // Items pushed: 30 (at ~200ms), 40 (at ~300ms), 50 (at ~400ms), 60 (at ~500ms), 70 (at ~600ms)
    // Relative to current time (now ~700ms from start of pushes)
    // Cutoff is now - 450ms = ~250ms from start.
    // So, 30 (pushed at ~200ms) should be expired.
    auto cutoff_time = Clock::now() - 450ms;
    ring.expire_older_than(cutoff_time);

    std::cout << "Size after expiration: " << ring.size() << std::endl;
    auto entries_after_expire = ring.entries_between(Clock::time_point::min(), Clock::time_point::max());
    print_entries<int>(entries_after_expire, "All entries after expiration");


    // Push specific timestamp
    std::cout << "\nPushing value with a specific past timestamp...\n";
    auto past_time = Clock::now() - 1000ms; // 1 second ago
    ring.push_at(-99, past_time);
    std::cout << "Pushed -99 at a past time. Size: " << ring.size() << std::endl;

    auto entries_with_past = ring.entries_between(Clock::time_point::min(), Clock::time_point::max());
    print_entries<int>(entries_with_past, "All entries after pushing -99 with past timestamp");
    std::cout << "Note: -99 might have overwritten an existing recent item if buffer was full.\n";

    std::cout << "\nQuerying items between a specific window...\n";
    // Assuming current items are 40,50,60,70 or similar, plus -99 (very old)
    // Let's try to get items pushed between 350ms and 550ms from the 'start' of the test sequence
    // This would roughly be 40 and 50
    // This is tricky because Clock::now() keeps moving.
    // For a stable test, we'd fix the 'now' point or use explicit timestamps for all.
    // The current `entries_between` uses absolute time points.

    // Let's find the timestamp of value "40" (originally pushed around 300ms from start)
    // and "50" (originally pushed around 400ms from start)
    // This is hard to get exact from outside without knowing the exact start time.
    // The example mainly demonstrates API usage.

    std::cout << "\nClearing the ring...\n";
    ring.clear();
    std::cout << "Size after clear: " << ring.size() << ", empty: " << ring.empty() << std::endl;
    auto entries_after_clear = ring.entries_between(Clock::time_point::min(), Clock::time_point::max());
    print_entries<int>(entries_after_clear, "All entries after clear");

    std::cout << "\nExample Finished.\n";

    return 0;
}
