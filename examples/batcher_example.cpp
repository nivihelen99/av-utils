#include "../include/batcher.h" // Adjust path if needed
#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <deque>

// Example functions (copied from batcher.h)
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

int main() {
    example_basic_usage();
    example_different_containers();
    example_edge_cases();
    example_const_container();
    example_batch_view_info();
    return 0;
}
