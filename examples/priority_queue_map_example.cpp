#include "PriorityQueueMap.h"
#include <iostream>
#include <string>
#include <vector>

void print_divider() {
    std::cout << "\n----------------------------------------\n" << std::endl;
}

int main() {
    std::cout << "PriorityQueueMap Example" << std::endl;
    print_divider();

    // --- Min-Priority Queue Example (default behavior) ---
    std::cout << "--- Min-Priority Queue (int keys, string values, int priorities) ---" << std::endl;
    cpp_utils::PriorityQueueMap<int, std::string, int> min_pq;

    std::cout << "Is empty initially? " << (min_pq.empty() ? "Yes" : "No") << ", Size: " << min_pq.size() << std::endl;

    min_pq.push(1, "Task A (Report)", 20);
    min_pq.push(2, "Task B (Bugfix)", 10); // Lower priority value = higher actual priority for min-heap
    min_pq.push(3, "Task C (Meeting)", 15);
    min_pq.push(4, "Task D (Research)", 10); // Same priority as Task B

    std::cout << "After pushes: Is empty? " << (min_pq.empty() ? "Yes" : "No") << ", Size: " << min_pq.size() << std::endl;

    if (!min_pq.empty()) {
        std::cout << "Top element: Key=" << min_pq.top_key()
                  << ", Priority=" << min_pq.top_priority()
                  << ", Value=\"" << min_pq.get_value(min_pq.top_key()) << "\"" << std::endl;
    }

    std::cout << "\nPushing existing key 1 with new value and priority (5):" << std::endl;
    min_pq.push(1, "Task A v2 (Urgent Report)", 5); // Updates key 1
    std::cout << "Top element after update: Key=" << min_pq.top_key()
              << ", Priority=" << min_pq.top_priority()
              << ", Value=\"" << min_pq.get_value(min_pq.top_key()) << "\"" << std::endl;

    std::cout << "\nUpdating priority of key 3 (Task C) to 2:" << std::endl;
    min_pq.update_priority(3, 2);
    std::cout << "Top element after update: Key=" << min_pq.top_key()
              << ", Priority=" << min_pq.top_priority()
              << ", Value=\"" << min_pq.get_value(min_pq.top_key()) << "\"" << std::endl;


    std::cout << "\nContains key 2? " << (min_pq.contains(2) ? "Yes" : "No") << std::endl;
    std::cout << "Contains key 99? " << (min_pq.contains(99) ? "Yes" : "No") << std::endl;

    std::cout << "\nRemoving key 2 (Task B):" << std::endl;
    min_pq.remove(2);
    std::cout << "Size after removing key 2: " << min_pq.size() << std::endl;
    std::cout << "Contains key 2 after removal? " << (min_pq.contains(2) ? "Yes" : "No") << std::endl;
    if (!min_pq.empty()) {
        std::cout << "Top element after removing key 2: Key=" << min_pq.top_key()
                  << ", Priority=" << min_pq.top_priority() << std::endl;
    }

    std::cout << "\nProcessing elements in priority order (min-heap):" << std::endl;
    while (!min_pq.empty()) {
        int key = min_pq.top_key();
        std::string value = min_pq.get_value(key);
        // Pop returns the priority of the popped element
        int priority = min_pq.pop();
        std::cout << "  Popped: Key=" << key << ", Value=\"" << value << "\", Priority=" << priority << std::endl;
    }
    std::cout << "Is empty finally? " << (min_pq.empty() ? "Yes" : "No") << ", Size: " << min_pq.size() << std::endl;

    print_divider();

    // --- Max-Priority Queue Example ---
    std::cout << "--- Max-Priority Queue (string keys, string values, double priorities) ---" << std::endl;
    cpp_utils::PriorityQueueMap<std::string, std::string, double, std::less<double>> max_pq;

    max_pq.push("ALPHA", "System Alpha", 0.75);
    max_pq.push("BETA", "System Beta", 0.90); // Higher priority value = higher actual priority for max-heap
    max_pq.push("GAMMA", "System Gamma", 0.80);

    if (!max_pq.empty()) {
        std::cout << "Top element: Key=" << max_pq.top_key()
                  << ", Priority=" << max_pq.top_priority()
                  << ", Value=\"" << max_pq.get_value(max_pq.top_key()) << "\"" << std::endl;
    }

    std::cout << "\nUpdating priority of ALPHA to 0.95:" << std::endl;
    max_pq.update_priority("ALPHA", 0.95);
    if (!max_pq.empty()) {
        std::cout << "Top element after update: Key=" << max_pq.top_key()
                  << ", Priority=" << max_pq.top_priority() << std::endl;
    }

    std::cout << "\nProcessing elements in priority order (max-heap):" << std::endl;
    while (!max_pq.empty()) {
        std::string key = max_pq.top_key();
        std::string value = max_pq.get_value(key);
        double priority = max_pq.pop();
        std::cout << "  Popped: Key=" << key << ", Value=\"" << value << "\", Priority=" << priority << std::endl;
    }

    print_divider();
    std::cout << "Example finished." << std::endl;

    return 0;
}
