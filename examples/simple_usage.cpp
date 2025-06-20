#include <iostream>
#include <vector>
#include <string>
#include <functional> // For std::greater
#include "../heap_queue.h" // Adjust path if heap_queue.h is elsewhere relative to root

// Example struct
struct Event {
    int priority;
    std::string name;

    // For printing
    bool operator==(const Event& other) const {
        return priority == other.priority && name == other.name;
    }
};

std::ostream& operator<<(std::ostream& os, const Event& e) {
    os << "Event{priority=" << e.priority << ", name='" << e.name << "'}";
    return os;
}

// Key extractor for Event priority
auto event_priority_key = [](const Event& e) {
    return e.priority;
};

int main() {
    std::cout << "--- Min-Heap Example (int) ---" << std::endl;
    HeapQueue<int> min_heap_int;
    min_heap_int.push(5);
    min_heap_int.push(1);
    min_heap_int.push(9);
    min_heap_int.push(3);

    std::cout << "Min-heap (int) elements (popping):" << std::endl;
    while (!min_heap_int.empty()) {
        std::cout << "Top: " << min_heap_int.top() << std::endl;
        std::cout << "Popped: " << min_heap_int.pop() << std::endl;
    }
    std::cout << std::endl;

    std::cout << "--- Heapify Example (int) ---" << std::endl;
    std::vector<int> nums_to_heapify = {40, 20, 50, 10, 30};
    min_heap_int.heapify(nums_to_heapify);
    std::cout << "Heapified (int) elements (popping):" << std::endl;
    while (!min_heap_int.empty()) {
        std::cout << "Popped: " << min_heap_int.pop() << std::endl;
    }
    std::cout << std::endl;

    std::cout << "--- Min-Heap Example (Event struct with key function) ---" << std::endl;
    // Explicitly providing std::less for KeyType (int) for min-heap
    HeapQueue<Event, std::less<int>, decltype(event_priority_key)> event_min_heap(event_priority_key);

    event_min_heap.push({5, "Task A"});
    event_min_heap.push({1, "Task B (urgent)"});
    event_min_heap.push({9, "Task C"});
    event_min_heap.push({1, "Task D (also urgent)"}); // Test duplicate keys

    std::cout << "Min-heap (Event) elements (popping by priority):" << std::endl;
    while (!event_min_heap.empty()) {
        std::cout << "Popped: " << event_min_heap.pop() << std::endl;
    }
    std::cout << std::endl;

    std::cout << "--- Max-Heap Example (Event struct with key function) ---" << std::endl;
    // Using std::greater for KeyType (int) to make it a max-heap
    HeapQueue<Event, std::greater<int>, decltype(event_priority_key)> event_max_heap(event_priority_key, std::greater<int>());

    event_max_heap.push({5, "Task X"});
    event_max_heap.push({1, "Task Y (low prio)"});
    event_max_heap.push({9, "Task Z (high prio)"});
    event_max_heap.push({5, "Task W (medium prio)"});

    std::cout << "Max-heap (Event) elements (popping by priority):" << std::endl;
    while (!event_max_heap.empty()) {
        std::cout << "Popped: " << event_max_heap.pop() << std::endl;
    }
    std::cout << std::endl;

    std::cout << "--- Update Top Example ---" << std::endl;
    HeapQueue<int> update_heap;
    update_heap.push(10);
    update_heap.push(20);
    update_heap.push(5);
    std::cout << "Initial top: " << update_heap.top() << std::endl; // Should be 5
    int old_top = update_heap.update_top(15);
    std::cout << "Old top was: " << old_top << std::endl; // Should be 5
    std::cout << "New top after update_top(15): " << update_heap.top() << std::endl; // Should be 10
    std::cout << "Heap after update_top (popping):" << std::endl;
    while(!update_heap.empty()){
        std::cout << update_heap.pop() << " ";
    }
    std::cout << std::endl;

    return 0;
}
