#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>
#include <type_traits>

// Helper trait to check if type is callable
template<typename T>
struct is_callable {
    template<typename U>
    static auto test(int) -> decltype(std::declval<U>()(), std::true_type{});
    template<typename>
    static std::false_type test(...);
    
    static constexpr bool value = decltype(test<T>(0))::value;
};

// any_of function - returns true if any element is truthy or satisfies predicate
template<typename Container>
bool any_of(const Container& container) {
    return std::any_of(container.begin(), container.end(), 
                      [](const auto& elem) { return static_cast<bool>(elem); });
}

template<typename Container, typename Predicate>
bool any_of(const Container& container, Predicate pred) {
    return std::any_of(container.begin(), container.end(), pred);
}

// all_of function - returns true if all elements are truthy or satisfy predicate
template<typename Container>
bool all_of(const Container& container) {
    return std::all_of(container.begin(), container.end(), 
                      [](const auto& elem) { return static_cast<bool>(elem); });
}

template<typename Container, typename Predicate>
bool all_of(const Container& container, Predicate pred) {
    return std::all_of(container.begin(), container.end(), pred);
}

// none_of function - bonus utility
template<typename Container>
bool none_of(const Container& container) {
    return std::none_of(container.begin(), container.end(), 
                       [](const auto& elem) { return static_cast<bool>(elem); });
}

template<typename Container, typename Predicate>
bool none_of(const Container& container, Predicate pred) {
    return std::none_of(container.begin(), container.end(), pred);
}

// Demo and test functions
// void demo_basic_usage() {
//     std::cout << "=== Basic Usage Demo ===\n";
    
//     // Test with integers (0 is falsy, non-zero is truthy)
//     std::vector<int> numbers1 = {1, 2, 3, 4, 5};
//     std::vector<int> numbers2 = {0, 2, 3, 4, 5};
//     std::vector<int> numbers3 = {0, 0, 0};
//     std::vector<int> empty_vec = {};
    
//     std::cout << "numbers1 = {1, 2, 3, 4, 5}\n";
//     std::cout << "any_of(numbers1): " << std::boolalpha << any_of(numbers1) << "\n";
//     std::cout << "all_of(numbers1): " << std::boolalpha << all_of(numbers1) << "\n\n";
    
//     std::cout << "numbers2 = {0, 2, 3, 4, 5}\n";
//     std::cout << "any_of(numbers2): " << std::boolalpha << any_of(numbers2) << "\n";
//     std::cout << "all_of(numbers2): " << std::boolalpha << all_of(numbers2) << "\n\n";
    
//     std::cout << "numbers3 = {0, 0, 0}\n";
//     std::cout << "any_of(numbers3): " << std::boolalpha << any_of(numbers3) << "\n";
//     std::cout << "all_of(numbers3): " << std::boolalpha << all_of(numbers3) << "\n\n";
    
//     std::cout << "empty_vec = {}\n";
//     std::cout << "any_of(empty_vec): " << std::boolalpha << any_of(empty_vec) << "\n";
//     std::cout << "all_of(empty_vec): " << std::boolalpha << all_of(empty_vec) << "\n\n";
// }

// void demo_with_predicates() {
//     std::cout << "=== Predicate Usage Demo ===\n";
    
//     std::vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
//     std::cout << "numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}\n";
    
//     // Check if any/all are even
//     auto is_even = [](int x) { return x % 2 == 0; };
//     std::cout << "any_of(numbers, is_even): " << std::boolalpha << any_of(numbers, is_even) << "\n";
//     std::cout << "all_of(numbers, is_even): " << std::boolalpha << all_of(numbers, is_even) << "\n\n";
    
//     // Check if any/all are greater than 5
//     auto greater_than_5 = [](int x) { return x > 5; };
//     std::cout << "any_of(numbers, >5): " << std::boolalpha << any_of(numbers, greater_than_5) << "\n";
//     std::cout << "all_of(numbers, >5): " << std::boolalpha << all_of(numbers, greater_than_5) << "\n\n";
    
//     // Check if any/all are positive
//     auto is_positive = [](int x) { return x > 0; };
//     std::cout << "any_of(numbers, >0): " << std::boolalpha << any_of(numbers, is_positive) << "\n";
//     std::cout << "all_of(numbers, >0): " << std::boolalpha << all_of(numbers, is_positive) << "\n\n";
// }

// void demo_string_usage() {
//     std::cout << "=== String Usage Demo ===\n";
    
//     std::vector<std::string> words1 = {"hello", "world", "test"};
//     std::vector<std::string> words2 = {"", "world", "test"};
//     std::vector<std::string> words3 = {"", "", ""};
    
//     std::cout << "words1 = {\"hello\", \"world\", \"test\"}\n";
//     std::cout << "any_of(words1): " << std::boolalpha << any_of(words1) << "\n";
//     std::cout << "all_of(words1): " << std::boolalpha << all_of(words1) << "\n\n";
    
//     std::cout << "words2 = {\"\", \"world\", \"test\"}\n";
//     std::cout << "any_of(words2): " << std::boolalpha << any_of(words2) << "\n";
//     std::cout << "all_of(words2): " << std::boolalpha << all_of(words2) << "\n\n";
    
//     std::cout << "words3 = {\"\", \"\", \"\"}\n";
//     std::cout << "any_of(words3): " << std::boolalpha << any_of(words3) << "\n";
//     std::cout << "all_of(words3): " << std::boolalpha << all_of(words3) << "\n\n";
    
//     // With predicates
//     auto longer_than_4 = [](const std::string& s) { return s.length() > 4; };
//     std::cout << "any_of(words1, length>4): " << std::boolalpha << any_of(words1, longer_than_4) << "\n";
//     std::cout << "all_of(words1, length>4): " << std::boolalpha << all_of(words1, longer_than_4) << "\n\n";
// }

// void demo_boolean_usage() {
//     std::cout << "=== Boolean Usage Demo ===\n";
    
//     std::vector<bool> bools1 = {true, false, true};
//     std::vector<bool> bools2 = {true, true, true};
//     std::vector<bool> bools3 = {false, false, false};
    
//     std::cout << "bools1 = {true, false, true}\n";
//     std::cout << "any_of(bools1): " << std::boolalpha << any_of(bools1) << "\n";
//     std::cout << "all_of(bools1): " << std::boolalpha << all_of(bools1) << "\n";
//     std::cout << "none_of(bools1): " << std::boolalpha << none_of(bools1) << "\n\n";
    
//     std::cout << "bools2 = {true, true, true}\n";
//     std::cout << "any_of(bools2): " << std::boolalpha << any_of(bools2) << "\n";
//     std::cout << "all_of(bools2): " << std::boolalpha << all_of(bools2) << "\n";
//     std::cout << "none_of(bools2): " << std::boolalpha << none_of(bools2) << "\n\n";
    
//     std::cout << "bools3 = {false, false, false}\n";
//     std::cout << "any_of(bools3): " << std::boolalpha << any_of(bools3) << "\n";
//     std::cout << "all_of(bools3): " << std::boolalpha << all_of(bools3) << "\n";
//     std::cout << "none_of(bools3): " << std::boolalpha << none_of(bools3) << "\n\n";
// }

// int main() {
//     demo_basic_usage();
//     demo_with_predicates();
//     demo_string_usage();
//     demo_boolean_usage();
    
//     return 0;
// }
