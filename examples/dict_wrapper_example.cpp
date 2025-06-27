#include "dict_wrapper.h" // Adjust path as necessary
#include <iostream>
#include <stdexcept> // For std::out_of_range
#include <string>

// Custom dictionary that logs insertions and deletions
class LoggingDict : public DictWrapper<std::string, int> {
public:
  using Base = DictWrapper<std::string, int>; // Base class alias

  // Inherit constructors from Base
  using Base::Base;

  // Override insert to log
  std::pair<iterator, bool> insert(const value_type &value) override {
    std::cout << "LOG: Inserting key='" << value.first
              << "', value=" << value.second << std::endl;
    auto result = Base::insert(value);
    if (result.second) {
      std::cout << "LOG: Successfully inserted key='" << value.first << "'"
                << std::endl;
    } else {
      std::cout << "LOG: Key='" << value.first
                << "' already exists. Insertion failed." << std::endl;
    }
    return result;
  }

  // Override erase (by key) to log
  size_type erase(const key_type &key) override {
    std::cout << "LOG: Attempting to erase key='" << key << "'" << std::endl;
    bool was_present = this->contains(key); // Check before erasing
    size_type result = Base::erase(key);
    if (result > 0) {
      std::cout << "LOG: Successfully erased key='" << key << "'" << std::endl;
    } else {
      if (was_present) { // Should not happen if erase returns 0 but was present
        std::cout << "LOG: Erase for key='" << key
                  << "' returned 0, but key was present. Inconsistent state?"
                  << std::endl;
      } else {
        std::cout << "LOG: Key='" << key << "' not found for erasure."
                  << std::endl;
      }
    }
    return result;
  }

  // Override operator[] to add custom behavior (e.g., logging access)
  mapped_type &operator[](const key_type &key) override {
    std::cout << "LOG: Accessing key='" << key << "' via operator[]"
              << std::endl;
    if (!this->contains(key)) {
      std::cout << "LOG: Key='" << key
                << "' not found by operator[], will be default-inserted."
                << std::endl;
    }
    return Base::operator[](key);
  }

  // Override at to log access (const version)
  const mapped_type &at(const key_type &key) const override {
    std::cout << "LOG: Accessing key='" << key << "' via at() const"
              << std::endl;
    try {
      return Base::at(key);
    } catch (const std::out_of_range &e) {
      std::cout << "LOG: Key='" << key
                << "' not found in at() const. Exception: " << e.what()
                << std::endl;
      throw; // Re-throw the exception
    }
  }
  // Override at to log access (non-const version)
  mapped_type &at(const key_type &key) override {
    std::cout << "LOG: Accessing key='" << key << "' via at() non-const"
              << std::endl;
    try {
      return Base::at(key);
    } catch (const std::out_of_range &e) {
      std::cout << "LOG: Key='" << key
                << "' not found in at() non-const. Exception: " << e.what()
                << std::endl;
      throw; // Re-throw the exception
    }
  }
};

int main() {
  std::cout << "--- Basic DictWrapper Usage ---" << std::endl;
  DictWrapper<std::string, int> basic_dict;
  basic_dict.insert({"one", 1});
  basic_dict["two"] = 2;

  std::cout << "basic_dict contains:" << std::endl;
  for (const auto &pair : basic_dict) {
    std::cout << pair.first << ": " << pair.second << std::endl;
  }
  std::cout << "Size of basic_dict: " << basic_dict.size() << std::endl;
  std::cout << std::endl;

  std::cout << "--- LoggingDict Usage ---" << std::endl;
  LoggingDict my_log_dict;

  // Test overridden insert
  my_log_dict.insert({"apple", 10});
  my_log_dict.insert({"banana", 20});
  my_log_dict.insert({"apple", 15}); // Attempt to insert duplicate

  std::cout << std::endl;
  // Test overridden operator[]
  my_log_dict["cherry"] = 30; // New element
  std::cout << "Value of cherry: " << my_log_dict["cherry"]
            << std::endl; // Existing element

  std::cout << std::endl;
  // Test overridden at
  try {
    std::cout << "Value of banana: " << my_log_dict.at("banana") << std::endl;
    std::cout << "Value of orange (const at): "
              << static_cast<const LoggingDict &>(my_log_dict).at("orange")
              << std::endl; // Test const at
  } catch (const std::out_of_range &e) {
    std::cout << "Exception caught: " << e.what() << std::endl;
  }

  std::cout << std::endl;
  std::cout << "Current LoggingDict state:" << std::endl;
  for (const auto &pair : my_log_dict) {
    std::cout << pair.first << ": " << pair.second << std::endl;
  }
  std::cout << "Size of my_log_dict: " << my_log_dict.size() << std::endl;

  std::cout << std::endl;
  // Test overridden erase
  my_log_dict.erase("banana");
  my_log_dict.erase("grape"); // Attempt to erase non-existent key

  std::cout << std::endl;
  std::cout << "LoggingDict state after erasures:" << std::endl;
  for (const auto &pair : my_log_dict) {
    std::cout << pair.first << ": " << pair.second << std::endl;
  }
  std::cout << "Size of my_log_dict: " << my_log_dict.size() << std::endl;

  std::cout << std::endl;
  std::cout
      << "--- Demonstrating other DictWrapper features with LoggingDict ---"
      << std::endl;
  LoggingDict dict2 = {{"uno", 1}, {"dos", 2}}; // Initializer list constructor
  std::cout << "dict2 initialized. Size: " << dict2.size() << std::endl;

  dict2.emplace("tres", 3); // Emplace (should use base emplace, no logging for
                            // this one unless emplace is also overridden)
  std::cout << "After emplace('tres', 3), dict2['tres']: " << dict2.at("tres")
            << std::endl;

  if (dict2.contains("dos")) {
    std::cout << "dict2 contains 'dos'." << std::endl;
  }

  dict2.clear(); // Clear (should use base clear)
  std::cout << "After clear(), dict2 is empty: " << std::boolalpha
            << dict2.empty() << std::endl;

  return 0;
}
