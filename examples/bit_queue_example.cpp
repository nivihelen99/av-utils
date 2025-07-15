#include "bit_queue.h"
#include <iostream>

int main() {
    cpp_utils::BitQueue bq;

    // Enqueue some bits
    bq.push(true);
    bq.push(false);
    bq.push(true);
    bq.push(true);

    std::cout << "Initial size: " << bq.size() << std::endl;

    // Dequeue and print the bits
    std::cout << "Bits: ";
    while (!bq.empty()) {
        std::cout << bq.pop() << " ";
    }
    std::cout << std::endl;
    std::cout << "Final size: " << bq.size() << std::endl;

    // Enqueue a 10-bit value
    bq.push(0x2A, 10); // 0b0000101010
    std::cout << "\nEnqueued 10 bits (0x2A). Size: " << bq.size() << std::endl;

    // Dequeue the 10-bit value
    uint64_t val = bq.pop(10);
    std::cout << "Dequeued 10 bits: " << std::hex << "0x" << val << std::dec << std::endl;
    std::cout << "Final size: " << bq.size() << std::endl;

    return 0;
}
