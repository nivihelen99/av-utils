#pragma once

#include <vector>
#include <iterator>
#include <type_traits>
#include <cassert>

namespace batcher_utils {

template <typename Iterator>
class BatchIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::vector<typename std::iterator_traits<Iterator>::value_type>;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;

private:
    Iterator current_;
    Iterator end_;
    std::size_t chunk_size_;
    mutable value_type current_batch_; // mutable for lazy evaluation in operator*

public:
    BatchIterator(Iterator current, Iterator end, std::size_t chunk_size)
        : current_(current), end_(end), chunk_size_(chunk_size) {
        assert(chunk_size > 0 && "Chunk size must be greater than 0");
    }

    value_type operator*() const {
        current_batch_.clear();
        current_batch_.reserve(chunk_size_);
        
        Iterator temp = current_;
        for (std::size_t i = 0; i < chunk_size_ && temp != end_; ++i, ++temp) {
            current_batch_.push_back(*temp);
        }
        
        return current_batch_;
    }

    BatchIterator& operator++() {
        for (std::size_t i = 0; i < chunk_size_ && current_ != end_; ++i, ++current_) {
            // Advance iterator by chunk_size or until end
        }
        return *this;
    }

    BatchIterator operator++(int) {
        BatchIterator temp = *this;
        ++(*this);
        return temp;
    }

    bool operator==(const BatchIterator& other) const {
        return current_ == other.current_;
    }

    bool operator!=(const BatchIterator& other) const {
        return !(*this == other);
    }

    // For debugging and introspection
    Iterator current() const { return current_; }
    std::size_t chunk_size() const { return chunk_size_; }
};

template <typename Container>
class BatchView {
public:
    using iterator_type = typename Container::iterator;
    using const_iterator_type = typename Container::const_iterator;
    using iterator = BatchIterator<iterator_type>;
    using const_iterator = BatchIterator<const_iterator_type>;

private:
    Container& container_;
    std::size_t chunk_size_;

public:
    BatchView(Container& container, std::size_t chunk_size)
        : container_(container), chunk_size_(chunk_size) {
        assert(chunk_size > 0 && "Chunk size must be greater than 0");
    }

    iterator begin() {
        return iterator(container_.begin(), container_.end(), chunk_size_);
    }

    iterator end() {
        return iterator(container_.end(), container_.end(), chunk_size_);
    }

    const_iterator begin() const {
        return const_iterator(container_.begin(), container_.end(), chunk_size_);
    }

    const_iterator end() const {
        return const_iterator(container_.end(), container_.end(), chunk_size_);
    }

    const_iterator cbegin() const {
        return begin();
    }

    const_iterator cend() const {
        return end();
    }

    // Optional: Calculate number of batches
    std::size_t size() const {
        auto container_size = std::distance(container_.begin(), container_.end());
        return container_size == 0 ? 0 : (container_size + chunk_size_ - 1) / chunk_size_;
    }

    bool empty() const {
        return container_.begin() == container_.end();
    }

    std::size_t chunk_size() const {
        return chunk_size_;
    }
};

// Const container specialization
template <typename Container>
class BatchView<const Container> {
public:
    using const_iterator_type = typename Container::const_iterator;
    using const_iterator = BatchIterator<const_iterator_type>;

private:
    const Container& container_;
    std::size_t chunk_size_;

public:
    BatchView(const Container& container, std::size_t chunk_size)
        : container_(container), chunk_size_(chunk_size) {
        assert(chunk_size > 0 && "Chunk size must be greater than 0");
    }

    const_iterator begin() const {
        return const_iterator(container_.begin(), container_.end(), chunk_size_);
    }

    const_iterator end() const {
        return const_iterator(container_.end(), container_.end(), chunk_size_);
    }

    const_iterator cbegin() const {
        return begin();
    }

    const_iterator cend() const {
        return end();
    }

    std::size_t size() const {
        auto container_size = std::distance(container_.begin(), container_.end());
        return container_size == 0 ? 0 : (container_size + chunk_size_ - 1) / chunk_size_;
    }

    bool empty() const {
        return container_.begin() == container_.end();
    }

    std::size_t chunk_size() const {
        return chunk_size_;
    }
};

} // namespace batcher_utils

// Factory function
template <typename Container>
auto batcher(Container& container, std::size_t chunk_size) {
    return batcher_utils::BatchView<Container>(container, chunk_size);
}

template <typename Container>
auto batcher(const Container& container, std::size_t chunk_size) {
    return batcher_utils::BatchView<const Container>(container, chunk_size);
}

// Convenience alias
template <typename Container>
using batch_view = batcher_utils::BatchView<Container>;

// Example usage and tests
#ifdef BATCHER_INCLUDE_EXAMPLES

#include <iostream>
#include <string>
#include <list>
#include <deque>

void example_basic_usage() {
    std::cout << "=== Basic Usage Example ===\n";
    
    std::vector<int> values = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    std::cout << "Original vector: ";
    for (int v : values) std::cout << v << " ";
    std::cout << "\n\nBatches of size 3:\n";
    
    for (const auto& batch : batcher(values, 3)) {
        std::cout << "Batch: ";
        for (int v : batch) std::cout << v << " ";
        std::cout << "\n";
    }
}

void example_different_containers() {
    std::cout << "\n=== Different Container Types ===\n";
    
    // List
    std::list<std::string> words = {"apple", "banana", "cherry", "date", "elderberry"};
    std::cout << "List batches (size 2):\n";
    for (const auto& batch : batcher(words, 2)) {
        std::cout << "Batch: ";
        for (const auto& word : batch) std::cout << word << " ";
        std::cout << "\n";
    }
    
    // Deque
    std::deque<char> chars = {'a', 'b', 'c', 'd', 'e', 'f', 'g'};
    std::cout << "\nDeque batches (size 4):\n";
    for (const auto& batch : batcher(chars, 4)) {
        std::cout << "Batch: ";
        for (char c : batch) std::cout << c << " ";
        std::cout << "\n";
    }
}

void example_edge_cases() {
    std::cout << "\n=== Edge Cases ===\n";
    
    // Empty container
    std::vector<int> empty_vec;
    std::cout << "Empty container batches: ";
    int count = 0;
    for (const auto& batch : batcher(empty_vec, 3)) {
        count++;
        std::cout << "Batch " << count << " (size " << batch.size() << ") ";
    }
    if (count == 0) std::cout << "No batches (as expected)";
    std::cout << "\n";
    
    // Single element
    std::vector<int> single = {42};
    std::cout << "Single element, chunk size 3: ";
    for (const auto& batch : batcher(single, 3)) {
        std::cout << "Batch: ";
        for (int v : batch) std::cout << v << " ";
        std::cout << "(size: " << batch.size() << ")";
    }
    std::cout << "\n";
    
    // Exact division
    std::vector<int> exact = {1, 2, 3, 4, 5, 6};
    std::cout << "Exact division (6 elements, chunk size 2):\n";
    for (const auto& batch : batcher(exact, 2)) {
        std::cout << "Batch: ";
        for (int v : batch) std::cout << v << " ";
        std::cout << "\n";
    }
}

void example_const_container() {
    std::cout << "\n=== Const Container Example ===\n";
    
    const std::vector<double> const_values = {1.1, 2.2, 3.3, 4.4, 5.5};
    
    std::cout << "Const vector batches (size 2):\n";
    for (const auto& batch : batcher(const_values, 2)) {
        std::cout << "Batch: ";
        for (double v : batch) std::cout << v << " ";
        std::cout << "\n";
    }
}

void example_batch_view_info() {
    std::cout << "\n=== BatchView Information ===\n";
    
    std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    auto batch_view = batcher(data, 4);
    
    std::cout << "Data size: " << data.size() << "\n";
    std::cout << "Chunk size: " << batch_view.chunk_size() << "\n";
    std::cout << "Number of batches: " << batch_view.size() << "\n";
    std::cout << "Is empty: " << (batch_view.empty() ? "yes" : "no") << "\n";
    
    std::cout << "Iterating through batches:\n";
    int batch_num = 1;
    for (const auto& batch : batch_view) {
        std::cout << "Batch " << batch_num << " (size " << batch.size() << "): ";
        for (int v : batch) std::cout << v << " ";
        std::cout << "\n";
        batch_num++;
    }
}

// Uncomment to run examples
/*
int main() {
    example_basic_usage();
    example_different_containers();
    example_edge_cases();
    example_const_container();
    example_batch_view_info();
    return 0;
}
*/

#endif // BATCHER_INCLUDE_EXAMPLES
