#include "vector_wrapper.h" // Adjust path as necessary
#include <iostream>
#include <string>
#include <vector>

// Example 1: Basic usage of VectorWrapper
void basic_usage_example() {
    std::cout << "--- Basic Usage Example ---" << std::endl;

    VectorWrapper<int> vec_wrapper;
    vec_wrapper.push_back(10);
    vec_wrapper.push_back(20);
    vec_wrapper.push_back(30);

    std::cout << "VectorWrapper contents: ";
    for (size_t i = 0; i < vec_wrapper.size(); ++i) {
        std::cout << vec_wrapper[i] << " ";
    }
    std::cout << std::endl;

    std::cout << "Front element: " << vec_wrapper.front() << std::endl;
    std::cout << "Back element: " << vec_wrapper.back() << std::endl;

    vec_wrapper.pop_back();
    std::cout << "After pop_back(), contents: ";
    for (const auto& val : vec_wrapper) { // Using range-based for
        std::cout << val << " ";
    }
    std::cout << std::endl;

    std::cout << "Size: " << vec_wrapper.size() << std::endl;
    std::cout << "Is empty? " << (vec_wrapper.empty() ? "Yes" : "No") << std::endl;

    VectorWrapper<std::string> str_vec_wrapper({"hello", "world"});
    str_vec_wrapper.insert(str_vec_wrapper.begin() + 1, "beautiful");
    std::cout << "String VectorWrapper: ";
    for (const auto& s : str_vec_wrapper) {
        std::cout << s << " ";
    }
    std::cout << std::endl;

    str_vec_wrapper.clear();
    std::cout << "After clear(), is string VectorWrapper empty? " << (str_vec_wrapper.empty() ? "Yes" : "No") << std::endl;
}

// Example 2: Deriving a custom class ObservableVector
template <typename T, typename InnerContainer = std::vector<T>>
class ObservableVector : public VectorWrapper<T, InnerContainer> {
public:
    // Inherit constructors from the base class
    using Base = VectorWrapper<T, InnerContainer>;
    using Base::Base; // This line inherits constructors in C++11 and later

    // Override push_back to log
    void push_back(const T& value) override {
        std::cout << "LOG: ObservableVector: push_back(" << value << ")" << std::endl;
        Base::push_back(value); // Call base class method
    }

    void push_back(T&& value) override {
        // To print the value, we might need to be careful if T is not copyable or expensive to copy for logging.
        // For simple types, this is fine.
        std::cout << "LOG: ObservableVector: push_back(moved value)" << std::endl;
        Base::push_back(std::move(value)); // Call base class method
    }

    // Override pop_back to log
    void pop_back() override {
        if (!this->empty()) {
            // For logging, it might be nice to log the value being popped.
            // This requires accessing the element before it's removed.
            std::cout << "LOG: ObservableVector: pop_back() on value " << this->back() << std::endl;
        } else {
            std::cout << "LOG: ObservableVector: pop_back() on empty vector" << std::endl;
        }
        Base::pop_back(); // Call base class method
    }

    // Override clear to log
    void clear() noexcept override {
        std::cout << "LOG: ObservableVector: clear()" << std::endl;
        Base::clear();
    }

    // Override insert to log
    typename Base::iterator insert(typename Base::const_iterator pos, const T& value) override {
        // Determine index for logging, if possible and useful
        // For simplicity, just log the value.
        std::cout << "LOG: ObservableVector: insert(" << value << ")" << std::endl;
        return Base::insert(pos, value);
    }

    // Override erase to log
    typename Base::iterator erase(typename Base::const_iterator pos) override {
        if (pos != this->end()) { // Use this->end() for dependent name lookup
             std::cout << "LOG: ObservableVector: erase() on value " << *pos << std::endl;
        } else {
             std::cout << "LOG: ObservableVector: erase() on end iterator" << std::endl;
        }
        return Base::erase(pos);
    }
};

void derived_class_example() {
    std::cout << "\n--- Derived Class Example (ObservableVector) ---" << std::endl;

    ObservableVector<int> obs_vec;
    obs_vec.push_back(100);
    obs_vec.push_back(200);

    std::cout << "ObservableVector contents: ";
    for (const auto& val : obs_vec) {
        std::cout << val << " ";
    }
    std::cout << std::endl;

    obs_vec.insert(obs_vec.begin() + 1, 150);
     std::cout << "ObservableVector contents after insert: ";
    for (const auto& val : obs_vec) {
        std::cout << val << " ";
    }
    std::cout << std::endl;

    obs_vec.pop_back();
    obs_vec.erase(obs_vec.begin());


    std::cout << "ObservableVector final contents: ";
    for (const auto& val : obs_vec) {
        std::cout << val << " ";
    }
    std::cout << std::endl;

    obs_vec.clear();
}

int main() {
    basic_usage_example();
    derived_class_example();
    return 0;
}
