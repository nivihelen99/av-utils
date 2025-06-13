#pragma once

#include <unordered_map>
#include <vector>
#include <algorithm>
#include <utility>
#include <iterator>
#include <initializer_list>
#include <functional>
#include <type_traits>

/**
 * @brief A generic frequency counter, similar to Python's collections.Counter
 * @tparam T The type of elements to count (must be hashable)
 * @tparam Hash Hash function for T (defaults to std::hash<T>)
 * @tparam KeyEqual Equality comparison for T (defaults to std::equal_to<T>)
 */
template<typename T, 
         typename Hash = std::hash<T>, 
         typename KeyEqual = std::equal_to<T>>
class Counter {
public:
    using key_type = T;
    using mapped_type = int;
    using value_type = std::pair<const T, int>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using hasher = Hash;
    using key_equal = KeyEqual;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;

private:
    using container_type = std::unordered_map<T, int, Hash, KeyEqual>;
    container_type counts_;

public:
    using iterator = typename container_type::iterator;
    using const_iterator = typename container_type::const_iterator;

    // Constructors
    Counter() = default;
    
    explicit Counter(size_type bucket_count, 
                    const Hash& hash = Hash{}, 
                    const KeyEqual& equal = KeyEqual{})
        : counts_(bucket_count, hash, equal) {}

    Counter(std::initializer_list<T> init) {
        counts_.reserve(init.size());
        for (const auto& val : init) {
            add(val);
        }
    }

    template<typename InputIt>
    Counter(InputIt first, InputIt last) {
        if constexpr (std::is_base_of_v<std::forward_iterator_tag, 
                      typename std::iterator_traits<InputIt>::iterator_category>) {
            counts_.reserve(std::distance(first, last));
        }
        for (auto it = first; it != last; ++it) {
            add(*it);
        }
    }

    Counter(std::initializer_list<std::pair<T, int>> init) {
        counts_.reserve(init.size());
        for (const auto& [key, count] : init) {
            if (count > 0) {
                counts_[key] = count;
            }
        }
    }

    // Copy and move constructors/assignment operators
    Counter(const Counter&) = default;
    Counter(Counter&&) = default;
    Counter& operator=(const Counter&) = default;
    Counter& operator=(Counter&&) = default;

    // Core operations
    void add(const T& value, int count = 1) {
        if (count > 0) {
            counts_[value] += count;
        } else if (count < 0) {
            subtract(value, -count);
        }
    }

    void add(T&& value, int count = 1) {
        if (count > 0) {
            counts_[std::move(value)] += count;
        } else if (count < 0) {
            subtract(std::move(value), -count);
        }
    }

    void subtract(const T& value, int count = 1) {
        if (count <= 0) return;
        
        auto it = counts_.find(value);
        if (it != counts_.end()) {
            it->second -= count;
            if (it->second <= 0) {
                counts_.erase(it);
            }
        }
    }

    [[nodiscard]] int count(const T& value) const noexcept {
        if (const auto it = counts_.find(value); it != counts_.end()) {
            return it->second;
        }
        return 0;
    }

    [[nodiscard]] int operator[](const T& value) const noexcept {
        return count(value);
    }

    // Non-const operator[] for modification
    int& operator[](const T& value) {
        return counts_[value];
    }

    [[nodiscard]] bool contains(const T& value) const noexcept {
        return counts_.find(value) != counts_.end();
    }

    size_type erase(const T& value) {
        return counts_.erase(value);
    }

    // Deprecated: use erase() instead
    [[deprecated("Use erase() instead of remove()")]]
    void remove(const T& value) {
        erase(value);
    }

    void clear() noexcept {
        counts_.clear();
    }

    [[nodiscard]] size_type size() const noexcept {
        return counts_.size();
    }

    [[nodiscard]] bool empty() const noexcept {
        return counts_.empty();
    }

    // Iterator support
    [[nodiscard]] iterator begin() noexcept { return counts_.begin(); }
    [[nodiscard]] iterator end() noexcept { return counts_.end(); }
    [[nodiscard]] const_iterator begin() const noexcept { return counts_.begin(); }
    [[nodiscard]] const_iterator end() const noexcept { return counts_.end(); }
    [[nodiscard]] const_iterator cbegin() const noexcept { return counts_.cbegin(); }
    [[nodiscard]] const_iterator cend() const noexcept { return counts_.cend(); }

    // Data access
    [[nodiscard]] const container_type& data() const noexcept {
        return counts_;
    }

    // Get copy of internal data (for backwards compatibility)
    [[nodiscard]] container_type get_data() const {
        return counts_;
    }

    [[nodiscard]] std::vector<std::pair<T, int>> most_common(size_type n = 0) const {
        std::vector<std::pair<T, int>> items;
        items.reserve(counts_.size());
        
        for (const auto& [key, count] : counts_) {
            items.emplace_back(key, count);
        }
        
        // Use partial sort if we only need top n elements
        if (n > 0 && n < items.size()) {
            std::partial_sort(items.begin(), items.begin() + n, items.end(),
                            [](const auto& a, const auto& b) {
                                if (a.second != b.second) {
                                    return a.second > b.second;
                                }
                                return a.first < b.first; // Stable ordering for ties
                            });
            items.resize(n);
        } else {
            std::sort(items.begin(), items.end(),
                     [](const auto& a, const auto& b) {
                         if (a.second != b.second) {
                             return a.second > b.second;
                         }
                         return a.first < b.first; // Stable ordering for ties
                     });
        }
        
        return items;
    }

    // Arithmetic operations
    Counter& operator+=(const Counter& other) {
        for (const auto& [key, count] : other.counts_) {
            counts_[key] += count;
        }
        return *this;
    }

    Counter& operator-=(const Counter& other) {
        for (const auto& [key, count] : other.counts_) {
            subtract(key, count);
        }
        return *this;
    }

    [[nodiscard]] Counter operator+(const Counter& other) const {
        Counter result = *this;
        result += other;
        return result;
    }

    [[nodiscard]] Counter operator-(const Counter& other) const {
        Counter result = *this;
        result -= other;
        return result;
    }

    // Comparison operators
    [[nodiscard]] bool operator==(const Counter& other) const noexcept {
        return counts_ == other.counts_;
    }

    [[nodiscard]] bool operator!=(const Counter& other) const noexcept {
        return !(*this == other);
    }

    // Utility methods
    [[nodiscard]] int total() const noexcept {
        int sum = 0;
        for (const auto& [key, count] : counts_) {
            sum += count;
        }
        return sum;
    }

    void reserve(size_type n) {
        counts_.reserve(n);
    }

    [[nodiscard]] size_type bucket_count() const noexcept {
        return counts_.bucket_count();
    }

    [[nodiscard]] double load_factor() const noexcept {
        return counts_.load_factor();
    }

    [[nodiscard]] double max_load_factor() const noexcept {
        return counts_.max_load_factor();
    }

    void max_load_factor(double ml) {
        counts_.max_load_factor(ml);
    }

    void rehash(size_type n) {
        counts_.rehash(n);
    }

    // Set operations
    [[nodiscard]] Counter intersection(const Counter& other) const {
        Counter result;
        for (const auto& [key, count] : counts_) {
            if (auto it = other.counts_.find(key); it != other.counts_.end()) {
                result.counts_[key] = std::min(count, it->second);
            }
        }
        return result;
    }

    [[nodiscard]] Counter union_with(const Counter& other) const {
        Counter result = *this;
        for (const auto& [key, count] : other.counts_) {
            auto& current = result.counts_[key];
            current = std::max(current, count);
        }
        return result;
    }

    // Filter operations
    template<typename Predicate>
    [[nodiscard]] Counter filter(Predicate pred) const {
        Counter result;
        for (const auto& [key, count] : counts_) {
            if (pred(key, count)) {
                result.counts_[key] = count;
            }
        }
        return result;
    }

    [[nodiscard]] Counter positive() const {
        return filter([](const T&, int count) { return count > 0; });
    }

    [[nodiscard]] Counter negative() const {
        return filter([](const T&, int count) { return count < 0; });
    }
};

// Deduction guides
template<typename InputIt>
Counter(InputIt, InputIt) -> Counter<typename std::iterator_traits<InputIt>::value_type>;

template<typename T>
Counter(std::initializer_list<T>) -> Counter<T>;

template<typename T>
Counter(std::initializer_list<std::pair<T, int>>) -> Counter<T>;
