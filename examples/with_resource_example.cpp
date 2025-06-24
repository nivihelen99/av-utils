#include "with_resource.h" // Adjust path as necessary
#include <fstream>
#include <iostream>
#include <vector>
#include <numeric> // For std::accumulate
#include <mutex>
#include <thread> // For std::this_thread::sleep_for
#include <chrono> // For std::chrono

// The example_usage function extracted from with_resource.h
void example_usage() {
    // File handling
    raii::with_resource(std::ofstream("test.txt"), [](auto& file) {
        if (!file.is_open()) {
            std::cerr << "Failed to open test.txt" << std::endl;
            return;
        }
        file << "Hello, World!" << std::endl;
        std::cout << "Successfully wrote to test.txt" << std::endl;
    });

    // Mutex locking
    std::mutex mtx;
    raii::with_resource(std::unique_lock<std::mutex>(mtx, std::defer_lock), [](auto& lock) {
        lock.lock(); // Lock explicitly
        std::cout << "In critical section" << std::endl;
        // lock.unlock(); // lock_guard would unlock in destructor, unique_lock too if owned
    }, 
    [](auto& lock){
        if (lock.owns_lock()){
            // std::cout << "Unlocking mutex in custom cleanup" << std::endl;
            lock.unlock();
        }
    });

    // Custom cleanup - Timer example
    auto start_time = std::chrono::steady_clock::now(); // Renamed to avoid conflict
    raii::with_resource(42, 
    [](auto& value) {
        std::cout << "Processing value: " << value << std::endl;
        // Simulate work
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }, 
    [start_time](auto& /*value*/) { // Capture start_time
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        std::cout << "Operation took: " << duration.count() << "ms" << std::endl;
    });

    // With return value
    int result = raii::with_resource_returning(std::vector<int>{1, 2, 3, 4, 5}, [](auto& vec) {
        return std::accumulate(vec.begin(), vec.end(), 0);
    });
    std::cout << "Sum: " << result << std::endl;

    // Using the macro for cleaner syntax - Reverted to direct call due to build issues
    // WITH_RESOURCE(std::ofstream("macro_test.txt"), file) {
    raii::with_resource(std::ofstream("macro_test.txt"), [&](auto& file) {
        if (!file.is_open()) {
            std::cerr << "Failed to open macro_test.txt" << std::endl;
            // return; // Cannot return from lambda like this easily outside main/void func
        } else {
            file << "Using macro syntax" << std::endl;
            std::cout << "Successfully wrote to macro_test.txt" << std::endl;
        }
    });

    // Example with resource that might fail to initialize
    // and custom cleanup for it
    bool resource_init_succeeded = false;
    raii::with_resource(
        // Resource factory lambda
        [&]() -> FILE* {
            FILE* fp = fopen("example_file.txt", "w");
            if (fp) {
                resource_init_succeeded = true;
                std::cout << "example_file.txt opened successfully." << std::endl;
            } else {
                resource_init_succeeded = false;
                std::cerr << "Failed to open example_file.txt for writing." << std::endl;
            }
            return fp;
        }(),
        // Main function lambda
        [](FILE* fp) {
            if (fp) {
                fprintf(fp, "Hello from with_resource!\n");
                std::cout << "Wrote to example_file.txt" << std::endl;
            } else {
                std::cout << "Skipping write as file pointer is null." << std::endl;
            }
        },
        // Cleanup lambda
        [&](FILE* fp) {
            if (fp) {
                fclose(fp);
                std::cout << "example_file.txt closed." << std::endl;
                // Optionally, remove the file if it was created by this example
                if (resource_init_succeeded) {
                    remove("example_file.txt");
                    std::cout << "example_file.txt removed." << std::endl;
                }
            } else {
                std::cout << "No file to close." << std::endl;
            }
        }
    );


    // Example demonstrating void return with custom cleanup
    std::cout << "Demonstrating void return with custom cleanup:" << std::endl;
    raii::with_resource_returning(
        std::string("TestResource"), 
        [](std::string& res) {
            std::cout << "Operating on resource: " << res << std::endl;
            // No return statement here, so it's a void return
        },
        [](std::string& res) {
            std::cout << "Custom cleanup for resource: " << res << std::endl;
        }
    );
    std::cout << "Void return example finished." << std::endl;

}

int main() {
    example_usage();
    return 0;
}
