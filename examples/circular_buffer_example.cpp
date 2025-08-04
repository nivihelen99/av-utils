#include "circular_buffer.h"
#include <iostream>
#include <string>

void print_buffer(const CircularBuffer<std::string>& buffer) {
    std::cout << "Buffer (size " << buffer.size() << "/" << buffer.capacity() << "): ";
    for (const auto& item : buffer) {
        std::cout << item << " ";
    }
    std::cout << std::endl;
}

int main() {
    std::cout << "Creating a CircularBuffer of strings with capacity 5." << std::endl;
    CircularBuffer<std::string> buffer(5);

    std::cout << "\nPushing elements to the back:" << std::endl;
    buffer.push_back("A");
    buffer.push_back("B");
    buffer.push_back("C");
    print_buffer(buffer);

    std::cout << "\nPushing more elements to fill the buffer:" << std::endl;
    buffer.push_back("D");
    buffer.push_back("E");
    print_buffer(buffer);

    std::cout << "\nPushing one more element ('F'), which overwrites the oldest ('A'):" << std::endl;
    buffer.push_back("F");
    print_buffer(buffer);
    std::cout << "Front is now: " << buffer.front() << ", Back is now: " << buffer.back() << std::endl;

    std::cout << "\nPopping from the front:" << std::endl;
    buffer.pop_front();
    print_buffer(buffer);

    std::cout << "\nPushing to the front ('G'):" << std::endl;
    buffer.push_front("G");
    print_buffer(buffer);
    std::cout << "Front is now: " << buffer.front() << ", Back is now: " << buffer.back() << std::endl;


    std::cout << "\n--- Testing Rotation ---" << std::endl;
    std::cout << "Initial state for rotation tests:" << std::endl;
    print_buffer(buffer);

    std::cout << "\nRotating by +2:" << std::endl;
    buffer.rotate(2);
    print_buffer(buffer);
    std::cout << "Front is now: " << buffer.front() << ", Back is now: " << buffer.back() << std::endl;
    std::cout << "Element at index 0: " << buffer[0] << ", Element at index 3: " << buffer[3] << std::endl;


    std::cout << "\nRotating by -1:" << std::endl;
    buffer.rotate(-1);
    print_buffer(buffer);
    std::cout << "Front is now: " << buffer.front() << ", Back is now: " << buffer.back() << std::endl;

    std::cout << "\nClearing the buffer." << std::endl;
    buffer.clear();
    print_buffer(buffer);

    return 0;
}
