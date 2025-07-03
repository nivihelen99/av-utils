#include <iostream>
#include <string>
#include "shadow_copy.h" // Assuming shadow_copy.h is in include directory

// Define a simple struct to use with ShadowCopy
struct MyData {
    int id;
    std::string description;

    // For easy printing
    friend std::ostream& operator<<(std::ostream& os, const MyData& data) {
        os << "ID: " << data.id << ", Description: '" << data.description << "'";
        return os;
    }

    // For comparison
    bool operator==(const MyData& other) const {
        return id == other.id && description == other.description;
    }
};

int main() {
    std::cout << "--- ShadowCopy Example ---" << std::endl;

    // 1. Initialize ShadowCopy with an original object
    MyData original_data = {1, "Initial version"};
    ShadowCopy<MyData> sc(original_data);

    std::cout << "Initial state:" << std::endl;
    std::cout << "  Original: " << sc.original() << std::endl;
    std::cout << "  Current:  " << sc.current() << std::endl;
    std::cout << "  Has shadow? " << (sc.has_shadow() ? "Yes" : "No") << std::endl;
    std::cout << "  Modified?   " << (sc.modified() ? "Yes" : "No") << std::endl;
    std::cout << std::endl;

    // 2. Get a mutable reference to the data (creates a shadow copy)
    std::cout << "Calling get() to create a shadow and modify..." << std::endl;
    MyData& mutable_data = sc.get();
    mutable_data.description = "Updated version";
    mutable_data.id = 2;

    std::cout << "After modification via get():" << std::endl;
    std::cout << "  Original: " << sc.original() << " (should be unchanged)" << std::endl;
    std::cout << "  Current:  " << sc.current() << " (should reflect changes)" << std::endl;
    std::cout << "  Has shadow? " << (sc.has_shadow() ? "Yes" : "No") << std::endl;
    std::cout << "  Modified?   " << (sc.modified() ? "Yes" : "No") << std::endl;
    std::cout << std::endl;

    // 3. Commit the changes (updates the original and removes the shadow)
    std::cout << "Calling commit()..." << std::endl;
    sc.commit();

    std::cout << "After commit():" << std::endl;
    std::cout << "  Original: " << sc.original() << " (should be the updated version)" << std::endl;
    std::cout << "  Current:  " << sc.current() << " (should be the updated version)" << std::endl;
    std::cout << "  Has shadow? " << (sc.has_shadow() ? "Yes" : "No") << std::endl;
    std::cout << "  Modified?   " << (sc.modified() ? "Yes" : "No") << std::endl;
    std::cout << std::endl;

    // 4. Make more changes
    std::cout << "Calling get() again for further modifications..." << std::endl;
    MyData& another_mutable_ref = sc.get();
    another_mutable_ref.description = "Final version after reset attempt";
    another_mutable_ref.id = 3;

    std::cout << "After second modification:" << std::endl;
    std::cout << "  Original: " << sc.original() << std::endl;
    std::cout << "  Current:  " << sc.current() << std::endl;
    std::cout << "  Has shadow? " << (sc.has_shadow() ? "Yes" : "No") << std::endl;
    std::cout << "  Modified?   " << (sc.modified() ? "Yes" : "No") << std::endl;
    std::cout << std::endl;

    // 5. Reset the changes (discards the shadow, reverts current to original)
    std::cout << "Calling reset()..." << std::endl;
    sc.reset();

    std::cout << "After reset():" << std::endl;
    std::cout << "  Original: " << sc.original() << " (should be version from last commit)" << std::endl;
    std::cout << "  Current:  " << sc.current() << " (should be version from last commit)" << std::endl;
    std::cout << "  Has shadow? " << (sc.has_shadow() ? "Yes" : "No") << std::endl;
    std::cout << "  Modified?   " << (sc.modified() ? "Yes" : "No") << std::endl;
    std::cout << std::endl;

    // 6. Take the shadow (if it exists)
    std::cout << "Modifying and then taking the shadow..." << std::endl;
    sc.get().description = "Value to be taken";
    sc.get().id = 99;
    
    if (sc.has_shadow()) {
        MyData taken_value = sc.take();
        std::cout << "Taken value: " << taken_value << std::endl;
    } else {
        std::cout << "No shadow to take (this shouldn't happen in this flow)." << std::endl;
    }

    std::cout << "After take():" << std::endl;
    std::cout << "  Original: " << sc.original() << std::endl;
    std::cout << "  Current:  " << sc.current() << std::endl;
    std::cout << "  Has shadow? " << (sc.has_shadow() ? "Yes" : "No") << std::endl;
    std::cout << "  Modified?   " << (sc.modified() ? "Yes" : "No") << std::endl;
    std::cout << std::endl;

    std::cout << "--- ShadowCopy Example Complete ---" << std::endl;
    return 0;
}
