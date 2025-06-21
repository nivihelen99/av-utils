#pragma once

#include <tuple>
#include <iterator>
#include <type_traits>

namespace zip_utils {

// Helper type traits for C++17
template <typename T>
using iterator_t = decltype(std::begin(std::declval<T&>()));

template <typename T>
using reference_t = decltype(*std::begin(std::declval<T&>()));

template <typename T>
using const_iterator_t = decltype(std::begin(std::declval<const T&>()));

template <typename T>
using const_reference_t = decltype(*std::begin(std::declval<const T&>()));

// Forward declaration
template <typename... Containers>
class ZippedView;

// Iterator implementation
template <typename... Containers>
class ZippedIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::tuple<reference_t<Containers>...>;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = value_type;

private:
    std::tuple<iterator_t<Containers>...> iterators_;

public:
    explicit ZippedIterator(iterator_t<Containers>... iters)
        : iterators_(std::move(iters)...) {}

    // Dereference operator - returns tuple of references
    auto operator*() const -> value_type {
        return dereference_impl(std::index_sequence_for<Containers...>{});
    }

    // Pre-increment operator
    ZippedIterator& operator++() {
        increment_impl(std::index_sequence_for<Containers...>{});
        return *this;
    }

    // Post-increment operator
    ZippedIterator operator++(int) {
        auto copy = *this;
        ++(*this);
        return copy;
    }

    // Equality comparison
    bool operator==(const ZippedIterator& other) const {
        return equals_impl(other, std::index_sequence_for<Containers...>{});
    }

    // Inequality comparison
    bool operator!=(const ZippedIterator& other) const {
        return !(*this == other);
    }

private:
    template <std::size_t... Is>
    auto dereference_impl(std::index_sequence<Is...>) const -> value_type {
        return std::forward_as_tuple(*std::get<Is>(iterators_)...);
    }

    template <std::size_t... Is>
    void increment_impl(std::index_sequence<Is...>) {
        (++std::get<Is>(iterators_), ...);
    }

    template <std::size_t... Is>
    bool equals_impl(const ZippedIterator& other, std::index_sequence<Is...>) const {
        // Iteration stops when ANY iterator reaches its end (shortest container)
        return ((std::get<Is>(iterators_) == std::get<Is>(other.iterators_)) || ...);
    }
};

// Const iterator implementation
template <typename... Containers>
class ZippedConstIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::tuple<const_reference_t<Containers>...>;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = value_type;

private:
    std::tuple<const_iterator_t<Containers>...> iterators_;

public:
    explicit ZippedConstIterator(const_iterator_t<Containers>... iters)
        : iterators_(std::move(iters)...) {}

    auto operator*() const -> value_type {
        return dereference_impl(std::index_sequence_for<Containers...>{});
    }

    ZippedConstIterator& operator++() {
        increment_impl(std::index_sequence_for<Containers...>{});
        return *this;
    }

    ZippedConstIterator operator++(int) {
        auto copy = *this;
        ++(*this);
        return copy;
    }

    bool operator==(const ZippedConstIterator& other) const {
        return equals_impl(other, std::index_sequence_for<Containers...>{});
    }

    bool operator!=(const ZippedConstIterator& other) const {
        return !(*this == other);
    }

private:
    template <std::size_t... Is>
    auto dereference_impl(std::index_sequence<Is...>) const -> value_type {
        return std::forward_as_tuple(*std::get<Is>(iterators_)...);
    }

    template <std::size_t... Is>
    void increment_impl(std::index_sequence<Is...>) {
        (++std::get<Is>(iterators_), ...);
    }

    template <std::size_t... Is>
    bool equals_impl(const ZippedConstIterator& other, std::index_sequence<Is...>) const {
        return ((std::get<Is>(iterators_) == std::get<Is>(other.iterators_)) || ...);
    }
};

// Main ZippedView class
template <typename... Containers>
class ZippedView {
public:
    using iterator = ZippedIterator<Containers...>;
    using const_iterator = ZippedConstIterator<Containers...>;

private:
    std::tuple<Containers&...> containers_;

public:
    explicit ZippedView(Containers&... containers)
        : containers_(containers...) {}

    // Begin iterator
    iterator begin() {
        return make_begin_iterator(std::index_sequence_for<Containers...>{});
    }

    // End iterator
    iterator end() {
        return make_end_iterator(std::index_sequence_for<Containers...>{});
    }

    // Const begin iterator
    const_iterator begin() const {
        return make_const_begin_iterator(std::index_sequence_for<Containers...>{});
    }

    // Const end iterator
    const_iterator end() const {
        return make_const_end_iterator(std::index_sequence_for<Containers...>{});
    }

    // Const begin iterator (cbegin)
    const_iterator cbegin() const {
        return begin();
    }

    // Const end iterator (cend)
    const_iterator cend() const {
        return end();
    }

private:
    template <std::size_t... Is>
    iterator make_begin_iterator(std::index_sequence<Is...>) {
        return iterator(std::begin(std::get<Is>(containers_))...);
    }

    template <std::size_t... Is>
    iterator make_end_iterator(std::index_sequence<Is...>) {
        return iterator(std::end(std::get<Is>(containers_))...);
    }

    template <std::size_t... Is>
    const_iterator make_const_begin_iterator(std::index_sequence<Is...>) const {
        return const_iterator(std::begin(std::get<Is>(containers_))...);
    }

    template <std::size_t... Is>
    const_iterator make_const_end_iterator(std::index_sequence<Is...>) const {
        return const_iterator(std::end(std::get<Is>(containers_))...);
    }
};

// Factory function for zip
template <typename... Containers>
auto zip(Containers&&... containers) -> ZippedView<std::remove_reference_t<Containers>...> {
    return ZippedView<std::remove_reference_t<Containers>...>(containers...);
}

// Bonus: Enumerate implementation
template <typename Container>
class EnumerateView {
public:
    using container_iterator = iterator_t<Container>;
    using value_type = std::pair<std::size_t, reference_t<Container>>;

    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::pair<std::size_t, reference_t<Container>>;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = value_type;

    private:
        std::size_t index_;
        container_iterator iter_;

    public:
        iterator(std::size_t index, container_iterator iter)
            : index_(index), iter_(iter) {}

        auto operator*() const -> value_type {
            return std::make_pair(index_, *iter_);
        }

        iterator& operator++() {
            ++index_;
            ++iter_;
            return *this;
        }

        iterator operator++(int) {
            auto copy = *this;
            ++(*this);
            return copy;
        }

        bool operator==(const iterator& other) const {
            return iter_ == other.iter_;
        }

        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }
    };

private:
    Container& container_;

public:
    explicit EnumerateView(Container& container) : container_(container) {}

    iterator begin() {
        return iterator(0, std::begin(container_));
    }

    iterator end() {
        return iterator(0, std::end(container_)); // Index doesn't matter for end
    }
};

// Factory function for enumerate
template <typename Container>
auto enumerate(Container&& container) -> EnumerateView<std::remove_reference_t<Container>> {
    return EnumerateView<std::remove_reference_t<Container>>(container);
}

} // namespace zip_utils

// Example usage and test cases
#ifdef ZIPPED_VIEW_EXAMPLE

#include <iostream>
#include <vector>
#include <list>
#include <deque>
#include <string>

int main() {
    using namespace zip_utils;

    // Example 1: Basic zip with vectors
    std::vector<int> ids = {1, 2, 3, 4};
    std::vector<std::string> names = {"one", "two", "three"};
    
    std::cout << "Example 1: Basic zip\n";
    for (auto&& [id, name] : zip(ids, names)) {
        std::cout << id << " = " << name << "\n";
    }

    // Example 2: Three containers with different types
    std::vector<int> a = {1, 2, 3};
    std::list<std::string> b = {"a", "b", "c"};
    std::deque<char> c = {'x', 'y', 'z'};
    
    std::cout << "\nExample 2: Three containers\n";
    for (auto&& [i, s, ch] : zip(a, b, c)) {
        std::cout << i << " " << s << " " << ch << "\n";
    }

    // Example 3: Mutable references
    std::vector<int> vec1 = {1, 2, 3};
    std::vector<int> vec2 = {10, 20, 30};
    
    std::cout << "\nExample 3: Before modification\n";
    for (auto&& [x, y] : zip(vec1, vec2)) {
        std::cout << x << " " << y << "\n";
    }
    
    // Modify through zip
    for (auto&& [x, y] : zip(vec1, vec2)) {
        x += y;
    }
    
    std::cout << "After modification (vec1 += vec2):\n";
    for (int x : vec1) {
        std::cout << x << " ";
    }
    std::cout << "\n";

    // Example 4: Enumerate
    std::vector<std::string> words = {"hello", "world", "cpp"};
    
    std::cout << "\nExample 4: Enumerate\n";
    for (auto&& [index, word] : enumerate(words)) {
        std::cout << index << ": " << word << "\n";
    }

    // Example 5: Const containers
    const std::vector<int> const_vec = {5, 6, 7};
    const std::vector<char> const_chars = {'a', 'b', 'c'};
    
    std::cout << "\nExample 5: Const containers\n";
    for (auto&& [num, ch] : zip(const_vec, const_chars)) {
        std::cout << num << " -> " << ch << "\n";
    }

    return 0;
}

#endif // ZIPPED_VIEW_EXAMPLE
