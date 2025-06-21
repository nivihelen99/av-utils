#include "round_robin_queue.h"
#include <iostream>
#include <string>
#include <vector>
#include <memory> // For std::shared_ptr if used in demo

// Function to demonstrate practical usage of RoundRobinQueue
void demonstrate_round_robin_queue() {
    std::cout << "=== RoundRobinQueue Demonstration ===" << std::endl;

    // Example 1: Basic string queue
    RoundRobinQueue<std::string> tasks;
    tasks.enqueue("Process emails");
    tasks.enqueue("Update database");
    tasks.enqueue("Generate reports");
    tasks.enqueue("Backup files");

    std::cout << "\nTask scheduler (round-robin):" << std::endl;
    for (int i = 0; i < 10; ++i) {
        if (tasks.empty()) {
            std::cout << "No tasks left." << std::endl;
            break;
        }
        std::cout << "Cycle " << i + 1 << ": Processing '" << tasks.next() << "'" << std::endl;
    }

    // Example 2: Load balancing simulation
    struct Server {
        std::string name;
        int load = 0;
        Server(std::string n) : name(std::move(n)) {}
        // Adding a stream operator for easy printing if needed
        friend std::ostream& operator<<(std::ostream& os, const Server& s) {
            return os << s.name << "(load: " << s.load << ")";
        }
    };

    RoundRobinQueue<Server> servers;
    servers.enqueue(Server("ServerAlpha"));
    servers.enqueue(Server("ServerBeta"));
    servers.enqueue(Server("ServerGamma"));

    std::cout << "\nLoad balancing simulation (10 requests):" << std::endl;
    for (int request = 0; request < 10; ++request) {
        if (servers.empty()) {
            std::cout << "No servers available." << std::endl;
            break;
        }
        Server& server_ref = servers.next();
        server_ref.load++;
        std::cout << "Request " << request + 1 << " assigned to " << server_ref.name
                  << ". Current load: " << server_ref.load << std::endl;
    }

    std::cout << "\nFinal server loads:" << std::endl;
    servers.for_each([](const Server& s) {
        std::cout << "- " << s.name << " has load: " << s.load << std::endl;
    });
    
    // Example 3: Using new features
    std::cout << "\nDemonstrating new features:" << std::endl;
    RoundRobinQueue<int> numbers;
    numbers.enqueue(10);
    numbers.enqueue(20);
    numbers.enqueue(30);
    numbers.enqueue(40);
    // Current: 10 (pos 0)

    std::cout << "Initial queue (current=" << numbers.peek() << "): ";
    numbers.for_each([](int n){ std::cout << n << " "; });
    std::cout << std::endl;

    numbers.rotate(2); // Rotate twice. Current: 10 -> 20 -> 30.
    std::cout << "After rotate(2) (current=" << numbers.peek() << "): ";
    numbers.for_each([](int n){ std::cout << n << " "; });
    std::cout << std::endl; // Expected: 30 40 10 20, current=30

    std::cout << "Contains 20? " << (numbers.contains(20) ? "Yes" : "No") << std::endl;
    std::cout << "Removing 20..." << std::endl;
    numbers.remove(20); // Removes 20. Queue: [10,30,40]. Current was 30. 20 was before 30. Current becomes 30.
                        // Q: [10,30,40] current points to 30. Original current was 30 (idx 2 of [10,20,30,40])
                        // After rotate(2) current was 2 (30).
                        // remove(20) -> 20 is at physical index 1. current=2. removed_idx=1 < current, so current--. current=1.
                        // Queue is [10,30,40]. current=1 points to 30.
    std::cout << "After removing 20 (current=" << numbers.peek() << "): ";
    numbers.for_each([](int n){ std::cout << n << " "; });
    std::cout << std::endl; // Expected: 10 30 40, current=30

    std::vector<int> initial_items = {100, 200, 300};
    RoundRobinQueue<int> from_vector(initial_items.begin(), initial_items.end());
    std::cout << "Queue from vector (current=" << from_vector.peek() << "): ";
    from_vector.for_each([](int n){ std::cout << n << " "; });
    std::cout << std::endl;
}

int main() {
    demonstrate_round_robin_queue();
    return 0;
}
