#include "TypeMap.h" // Assuming TypeMap.h is in the include path or same directory for simple examples
#include <iostream>
#include <string>
#include <vector>

// Custom struct for demonstration
struct MyStruct {
    int id;
    std::string data;

    MyStruct(int i, std::string d) : id(i), data(std::move(d)) {}

    // For easy printing
    friend std::ostream& operator<<(std::ostream& os, const MyStruct& s) {
        os << "MyStruct{id=" << s.id << ", data=\"" << s.data << "\"}";
        return os;
    }
};

class MyService {
public:
    MyService(std::string name) : service_name_(std::move(name)) {}

    void perform_action() const {
        std::cout << "Service '" << service_name_ << "' is performing an action." << std::endl;
    }

    std::string get_name() const {
        return service_name_;
    }
private:
    std::string service_name_;
};


int main() {
    cpp_utils::TypeMap type_map;

    std::cout << "Initial TypeMap size: " << type_map.size() << std::endl;
    std::cout << "Initial TypeMap empty: " << std::boolalpha << type_map.empty() << std::endl << std::endl;

    // 1. Putting different types
    std::cout << "--- Storing objects ---" << std::endl;
    type_map.put<int>(42);
    type_map.put<std::string>("Hello, TypeMap!");
    type_map.put<MyStruct>(MyStruct(101, "Test Data"));
    type_map.put<double>(3.14159);

    // Storing a more complex object, e.g., a shared_ptr to a service
    // Note: TypeMap stores a copy of what you put. If you want shared ownership, store a shared_ptr.
    auto service_ptr = std::make_shared<MyService>("LoggingService");
    // Pass service_ptr directly; T_param will be deduced as std::shared_ptr<MyService>&
    // std::forward will pass it as an lvalue, std::any will copy-construct the shared_ptr.
    type_map.put(service_ptr);


    std::cout << "TypeMap size after puts: " << type_map.size() << std::endl;
    std::cout << "TypeMap empty after puts: " << type_map.empty() << std::endl << std::endl;

    // 2. Getting objects
    std::cout << "--- Retrieving objects ---" << std::endl;
    // Using get<T>() -> returns T*
    if (int* i_ptr = type_map.get<int>()) {
        std::cout << "Retrieved int: " << *i_ptr << std::endl;
    }
    if (std::string* s_ptr = type_map.get<std::string>()) {
        std::cout << "Retrieved string: " << *s_ptr << std::endl;
    }
    if (MyStruct* ms_ptr = type_map.get<MyStruct>()) {
        std::cout << "Retrieved MyStruct: " << *ms_ptr << std::endl;
    }
    if (const double* d_ptr = type_map.get<double>()) { // Can get as const T*
        std::cout << "Retrieved double (const*): " << *d_ptr << std::endl;
    }

    // Using get_ref<T>() -> returns T& (throws if not found)
    try {
        std::cout << "Retrieved string by reference: " << type_map.get_ref<std::string>() << std::endl;
        MyStruct& ms_ref = type_map.get_ref<MyStruct>();
        ms_ref.data = "Updated Test Data"; // Modify through reference
        std::cout << "Modified MyStruct through reference: " << type_map.get_ref<MyStruct>() << std::endl;

        const int& i_const_ref = type_map.get_ref<int>(); // Can get as const T&
        std::cout << "Retrieved int (const&): " << i_const_ref << std::endl;

    } catch (const std::out_of_range& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
    }
    std::cout << std::endl;

    // Retrieving the service
    if (auto* service_sptr_ptr = type_map.get<std::shared_ptr<MyService>>()) {
        (*service_sptr_ptr)->perform_action();
        std::cout << "Service pointer still valid, name: " << (*service_sptr_ptr)->get_name() << std::endl;
    }


    // 3. Checking containment
    std::cout << "--- Checking containment ---" << std::endl;
    std::cout << "Contains int? " << std::boolalpha << type_map.contains<int>() << std::endl;
    std::cout << "Contains float? " << std::boolalpha << type_map.contains<float>() << std::endl;
    std::cout << "Contains MyStruct? " << std::boolalpha << type_map.contains<MyStruct>() << std::endl;
    std::cout << "Contains std::vector<int>? " << std::boolalpha << type_map.contains<std::vector<int>>() << std::endl;
    std::cout << std::endl;

    // 4. Overwriting an existing type
    std::cout << "--- Overwriting an object ---" << std::endl;
    std::cout << "Current int: " << type_map.get_ref<int>() << std::endl;
    type_map.put<int>(1000);
    std::cout << "Overwritten int: " << type_map.get_ref<int>() << std::endl;
    std::cout << "TypeMap size (should be unchanged): " << type_map.size() << std::endl << std::endl;

    // 5. Removing objects
    std::cout << "--- Removing objects ---" << std::endl;
    std::cout << "Attempting to remove float (should fail): " << type_map.remove<float>() << std::endl;
    std::cout << "Contains int before remove? " << type_map.contains<int>() << std::endl;
    std::cout << "Removing int (should succeed): " << type_map.remove<int>() << std::endl;
    std::cout << "Contains int after remove? " << type_map.contains<int>() << std::endl;
    std::cout << "TypeMap size after removing int: " << type_map.size() << std::endl;

    bool removed_string = type_map.remove<std::string>();
    std::cout << "Removed string: " << removed_string << ", New size: " << type_map.size() << std::endl;
    std::cout << std::endl;

    // 6. Handling non-existent types
    std::cout << "--- Handling non-existent types ---" << std::endl;
    if (type_map.get<char>() == nullptr) {
        std::cout << "char is not in the map (get<char>() returned nullptr)." << std::endl;
    }
    try {
        type_map.get_ref<unsigned long>();
    } catch (const std::out_of_range& e) {
        std::cerr << "Caught expected exception for get_ref<unsigned long>: " << e.what() << std::endl;
    }
    std::cout << std::endl;

    // 7. Clearing the map
    std::cout << "--- Clearing the map ---" << std::endl;
    std::cout << "TypeMap size before clear: " << type_map.size() << std::endl;
    type_map.clear();
    std::cout << "TypeMap size after clear: " << type_map.size() << std::endl;
    std::cout << "TypeMap empty after clear: " << std::boolalpha << type_map.empty() << std::endl;
    std::cout << "Contains MyStruct after clear? " << std::boolalpha << type_map.contains<MyStruct>() << std::endl;

    std::cout << "\nExample finished." << std::endl;

    return 0;
}
