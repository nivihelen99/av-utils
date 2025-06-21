#pragma once

#include <deque>
#include <stdexcept>
#include <functional>
#include <algorithm> // For std::find, std::remove
#include <iterator>  // For std::distance

/**
 * @brief A fair rotating queue for C++17 that provides circular access to elements
 * 
 * RoundRobinQueue is inspired by Python's deque + rotate() functionality and provides
 * efficient round-robin access patterns useful for load balancing, fair scheduling,
 * and rotating access across multiple resources.
 * 
 * @tparam T The type of elements stored in the queue
 */
template<typename T>
class RoundRobinQueue {
private:
    std::deque<T> queue;
    size_t current = 0;

public:
    /**
     * @brief Default constructor
     */
    RoundRobinQueue() = default;

    /**
     * @brief Constructor from a pair of iterators
     * @tparam InputIt Iterator type
     * @param first Iterator to the first element
     * @param last Iterator to the one past the last element
     */
    template<typename InputIt>
    RoundRobinQueue(InputIt first, InputIt last) : queue(first, last), current(0) {}

    /**
     * @brief Add an element to the back of the queue
     * @param item The item to add
     * @complexity O(1) amortized
     */
    void enqueue(const T& item) {
        queue.push_back(item);
    }

    /**
     * @brief Add an element to the back of the queue (move version)
     * @param item The item to move into the queue
     * @complexity O(1) amortized
     */
    void enqueue(T&& item) {
        queue.push_back(std::move(item));
    }

    /**
     * @brief Get the next element in round-robin order and advance the pointer
     * @return Reference to the current element
     * @throws std::runtime_error if queue is empty
     * @complexity O(1)
     */
    T& next() {
        if (empty()) {
            throw std::runtime_error("RoundRobinQueue is empty");
        }
        
        T& result = queue[current];
        current = (current + 1) % queue.size();
        return result;
    }

    /**
     * @brief Get the current element without advancing the pointer
     * @return Reference to the current element
     * @throws std::runtime_error if queue is empty
     * @complexity O(1)
     */
    T& peek() {
        if (empty()) {
            throw std::runtime_error("RoundRobinQueue is empty");
        }
        return queue[current];
    }

    /**
     * @brief Get the current element without advancing the pointer (const version)
     * @return Const reference to the current element
     * @throws std::runtime_error if queue is empty
     * @complexity O(1)
     */
    const T& peek() const {
        if (empty()) {
            throw std::runtime_error("RoundRobinQueue is empty");
        }
        return queue[current];
    }

    /**
     * @brief Remove the current element from the queue
     * @throws std::runtime_error if queue is empty
     * @complexity O(n) worst case, O(1) average for removal at ends
     */
    void skip() {
        if (empty()) {
            throw std::runtime_error("RoundRobinQueue is empty");
        }
        
        queue.erase(queue.begin() + current);
        
        // Adjust current pointer after removal
        if (queue.empty()) {
            current = 0;
        } else if (current >= queue.size()) {
            current = 0;
        }
    }

    /**
     * @brief Remove all elements from the queue
     * @complexity O(n)
     */
    void clear() {
        queue.clear();
        current = 0;
    }

    /**
     * @brief Check if the queue is empty
     * @return true if empty, false otherwise
     * @complexity O(1)
     */
    bool empty() const {
        return queue.empty();
    }

    /**
     * @brief Get the number of elements in the queue
     * @return Number of elements
     * @complexity O(1)
     */
    size_t size() const {
        return queue.size();
    }

    /**
     * @brief Reset the round-robin pointer to the beginning
     * @complexity O(1)
     */
    void reset() {
        current = 0;
    }

    /**
     * @brief Add an element to the front of the queue (priority insertion)
     * @param item The item to add to the front
     * @complexity O(1)
     */
    void insert_front(const T& item) {
        bool was_empty = queue.empty();
        queue.push_front(item);
        // Adjust current pointer since we inserted at the front.
        // If the queue was not empty, the previously current item is shifted one position to the right.
        if (!was_empty) {
            current++;
        }
        // If the queue was empty, current remains 0, which is correct.
    }

    /**
     * @brief Add an element to the front of the queue (move version)
     * @param item The item to move to the front
     * @complexity O(1)
     */
    void insert_front(T&& item) {
        bool was_empty = queue.empty();
        queue.push_front(std::move(item));
        // Adjust current pointer since we inserted at the front.
        if (!was_empty) {
            current++;
        }
    }

    /**
     * @brief Visit all elements in round-robin order without advancing the pointer
     * @param callback Function to call for each element
     * @complexity O(n)
     */
    template<typename Callback>
    void for_each(Callback callback) const {
        if (empty()) return;
        
        size_t pos = current;
        for (size_t i = 0; i < queue.size(); ++i) {
            callback(queue[pos]);
            pos = (pos + 1) % queue.size();
        }
    }

    /**
     * @brief Get the current round-robin position
     * @return Current position index
     * @complexity O(1)
     */
    size_t current_position() const {
        return current;
    }

    /**
     * @brief Rotate the queue's starting point.
     * Positive n advances the current pointer, negative n moves it backward.
     * @param n Number of positions to rotate.
     * @complexity O(1)
     */
    void rotate(int n) {
        if (empty()) {
            return;
        }
        // Modulo arithmetic needs to handle negative results correctly.
        // (current + n) % size might be negative if (current + n) is negative.
        // So, ((current + n) % size + size) % size ensures a positive result.
        long long num_elements = static_cast<long long>(queue.size());
        long long current_ll = static_cast<long long>(current);
        long long n_ll = static_cast<long long>(n);

        current = static_cast<size_t>(((current_ll + n_ll) % num_elements + num_elements) % num_elements);
    }

    /**
     * @brief Remove the first occurrence of a specific value from the queue.
     * @param value The value to remove.
     * @return True if an element was removed, false otherwise.
     * @complexity O(N) due to searching and potential erase in std::deque.
     */
    bool remove(const T& value) {
        if (empty()) {
            return false;
        }

        auto it = std::find(queue.begin(), queue.end(), value);
        if (it == queue.end()) {
            return false; // Value not found
        }

        size_t removed_idx = std::distance(queue.begin(), it);
        queue.erase(it);

        if (empty()) {
            current = 0;
        } else {
            if (removed_idx < current) {
                current--; // Element before current was removed
            } else if (removed_idx == current) {
                // Current element was removed, new current should be at the same "logical" position
                // which means it stays at the same index, unless it was the last element.
                if (current >= queue.size()) { // If it was the last element
                    current = 0;
                }
            }
            // If removed_idx > current, current index is not affected unless
            // current becomes out of bounds due to removal of last element,
            // which is handled by the (current >= queue.size()) check.
            if (current >= queue.size() && !queue.empty()) { // Ensure current is valid
                 current = 0; // or current % queue.size(); but 0 is safer if current was > size
            }
        }
        return true;
    }

    /**
     * @brief Check if the queue contains a specific value.
     * @param value The value to check for.
     * @return True if the value is found, false otherwise.
     * @complexity O(N)
     */
    bool contains(const T& value) const {
        return std::find(queue.begin(), queue.end(), value) != queue.end();
    }

    // Iterator support for STL compatibility
    using iterator = typename std::deque<T>::iterator;
    using const_iterator = typename std::deque<T>::const_iterator;

    /**
     * @brief Get iterator to beginning of underlying container
     * @return Iterator to beginning
     * @note This provides access to the underlying storage order, not round-robin order
     */
    iterator begin() { return queue.begin(); }
    const_iterator begin() const { return queue.begin(); }
    const_iterator cbegin() const { return queue.cbegin(); }

    /**
     * @brief Get iterator to end of underlying container
     * @return Iterator to end
     * @note This provides access to the underlying storage order, not round-robin order
     */
    iterator end() { return queue.end(); }
    const_iterator end() const { return queue.end(); }
    const_iterator cend() const { return queue.cend(); }
};

// Example usage and demonstration
#ifdef ROUND_ROBIN_QUEUE_EXAMPLE
#include <iostream>
#include <string>
#include <vector>

void demonstrate_round_robin_queue() {
    std::cout << "=== RoundRobinQueue Demo ===" << std::endl;
    
    // Basic usage
    RoundRobinQueue<std::string> rr;
    rr.enqueue("A");
    rr.enqueue("B");
    rr.enqueue("C");
    
    std::cout << "Round-robin access: ";
    for (int i = 0; i < 7; ++i) {
        std::cout << rr.next() << " ";
    }
    std::cout << std::endl;
    
    // Peek and skip demo
    RoundRobinQueue<int> numbers;
    for (int i = 1; i <= 5; ++i) {
        numbers.enqueue(i);
    }
    
    std::cout << "Before skip - peek: " << numbers.peek() << std::endl;
    numbers.skip(); // Remove current element
    std::cout << "After skip - next: " << numbers.next() << std::endl;
    
    // Load balancing example
    struct Server {
        std::string name;
        int load = 0;
        Server(const std::string& n) : name(n) {}
    };
    
    RoundRobinQueue<Server> servers;
    servers.enqueue(Server("Server1"));
    servers.enqueue(Server("Server2"));
    servers.enqueue(Server("Server3"));
    
    std::cout << "Load balancing simulation:" << std::endl;
    for (int request = 0; request < 10; ++request) {
        Server& server = servers.next();
        server.load++;
        std::cout << "Request " << request + 1 << " -> " << server.name 
                  << " (load: " << server.load << ")" << std::endl;
    }
    
    // for_each demonstration
    std::cout << "Final server loads: ";
    servers.for_each([](const Server& s) {
        std::cout << s.name << "(" << s.load << ") ";
    });
    std::cout << std::endl;
}
#endif
