#include "skiplist.h"
#include <iostream>
// --- MyData Struct and Utilities ---
#include <string>    // For std::string
#include <utility>   // For std::move (in constructor)
#include <vector>    // For tests
#include <algorithm> // For std::sort in tests
#include <functional>// For std::less in tests
#include <iomanip>   // For std::fixed, std::setprecision
#include <cmath>     // For std::abs (double comparison)
#include <sstream>   // For std::ostringstream in value_to_log_string

struct MyData; // Forward declare MyData
template<> std::string value_to_log_string<MyData>(const MyData& val); // Forward declare specialization

struct MyData {
    int id;
    std::string name;
    double score;
    bool is_active;

    MyData(int i = 0, std::string n = "", double s = 0.0, bool active = false)
        : id(i), name(std::move(n)), score(s), is_active(active) {}

    bool operator==(const MyData& other) const {
        return id == other.id &&
               name == other.name &&
               (std::abs(score - other.score) < 1e-9) &&
               is_active == other.is_active;
    }

    bool operator!=(const MyData& other) const {
        return !(*this == other);
    }

    bool operator<(const MyData& other) const {
        if (id != other.id) return id < other.id;
        if (name != other.name) return name < other.name;
        if (std::abs(score - other.score) >= 1e-9) { // If scores are significantly different
            return score < other.score;
        }
        // If scores are "equal" or very close, compare by is_active
        return is_active < other.is_active;
    }
};

template<>
std::string value_to_log_string<MyData>(const MyData& val) {
    std::ostringstream oss;
    oss << "MyData(id=" << val.id
        << ", name=\"" << val.name << "\""
        << ", score=" << std::fixed << std::setprecision(2) << val.score
        << ", active=" << (val.is_active ? "true" : "false") << ")";
    return oss.str();
}

namespace std {
    template<>
    struct less<MyData> {
        bool operator()(const MyData& lhs, const MyData& rhs) const {
            return lhs < rhs;
        }
    };
}
// --- End of MyData Struct and Utilities ---

int main() {
    std::cout << "--- SkipList with Custom Struct (std::pair<const int, MyData>) Example ---" << '\n';

    SkipList<std::pair<const int, MyData>> sl;

    MyData val1(1, "Alice", 95.5, true);
    MyData val2(2, "Bob", 88.0, true);
    MyData val3(3, "Charlie", 92.0, false);

    std::cout << "\nInserting items..." << '\n';
    sl.insert({val1.id, val1});
    sl.insert({val2.id, val2});
    sl.insert({val3.id, val3});
    sl.display();

    std::cout << "\nSearching for items..." << '\n';
    std::cout << "Search for key 2 (Bob): " << (sl.search({2, MyData()}) ? "Found" : "Not Found") << '\n';
    std::cout << "Search for key 4 (Non-existent): " << (sl.search({4, MyData()}) ? "Found" : "Not Found") << '\n';

    std::cout << "\nUsing find for key 1 (Alice)..." << '\n';
    auto it_alice = sl.find(1);
    if (it_alice != sl.end()) {
        std::cout << "Found: Key=" << it_alice->first << ", Value=" << value_to_log_string(it_alice->second) << '\n';
        it_alice->second.name = "Alicia Updated";
        it_alice->second.score = 96.88;
        std::cout << "Modified Alice's data: " << value_to_log_string(it_alice->second) << '\n';
    } else {
        std::cout << "Key 1 not found." << '\n';
    }
    sl.display();

    std::cout << "\nUsing insert_or_assign..." << '\n';
    MyData val2_updated(2, "Robert (Bob)", 89.55, true);
    auto result_bob = sl.insert_or_assign({val2_updated.id, val2_updated});
    std::cout << "Key 2 action: " << (result_bob.second ? "Inserted" : "Assigned")
              << ", Value: " << value_to_log_string(result_bob.first->second) << '\n';
    sl.display();

    MyData val4(4, "David", 75.25, true);
    auto result_david = sl.insert_or_assign({val4.id, val4});
    std::cout << "Key 4 action: " << (result_david.second ? "Inserted" : "Assigned")
              << ", Value: " << value_to_log_string(result_david.first->second) << '\n';
    sl.display();

    std::cout << "\nIterating through skiplist:" << '\n';
    for (const auto& pair_entry : sl) {
        std::cout << "Key: " << pair_entry.first << ", Value: " << value_to_log_string(pair_entry.second) << '\n';
    }

    std::cout << "\nRemoving item with key 3 (Charlie)..." << '\n';
    sl.remove({3, MyData()});
    sl.display();

    std::cout << "\nClearing skiplist..." << '\n';
    sl.clear();
    sl.display();
    std::cout << "Size after clear: " << sl.size() << '\n';

    std::cout << "\n--- Example End ---" << '\n';
    return 0;
}
