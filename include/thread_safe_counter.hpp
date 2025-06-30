#pragma once

#include <unordered_map>
#include <vector>
#include <algorithm>
#include <utility>
#include <iterator>
#include <initializer_list>
#include <functional>
#include <type_traits>
#include <mutex>
#include <thread> // For std::this_thread::get_id in debugging, if needed

// Helper to check if a type is std::pair (copied from counter.h)
template<typename>
struct is_std_pair_impl : std::false_type {};
template<typename T1, typename T2>
struct is_std_pair_impl<std::pair<T1, T2>> : std::true_type {};
template<typename T>
constexpr bool is_std_pair_v = is_std_pair_impl<std::remove_cv_t<T>>::value;

// Helper for std::is_lt_comparable_v (C++20 has std::totally_ordered, C++17 needs a helper)
namespace detail {
    template <typename U, typename = void> // Changed T to U to avoid conflict
    struct is_lt_comparable : std::false_type {};

    template <typename U> // Changed T to U to avoid conflict
    struct is_lt_comparable<U, std::void_t<decltype(std::declval<U>() < std::declval<U>())>> : std::true_type {};
} // namespace detail

template<typename U> // Changed T to U to avoid conflict
constexpr bool is_lt_comparable_v = detail::is_lt_comparable<U>::value;


/**
 * @brief A thread-safe generic frequency counter, similar to Python's collections.Counter.
 * This class uses a std::mutex to protect its internal data, allowing for safe
 * concurrent access and modification from multiple threads.
 *
 * @tparam T The type of elements to count (must be hashable)
 * @tparam Hash Hash function for T (defaults to std::hash<T>)
 * @tparam KeyEqual Equality comparison for T (defaults to std::equal_to<T>)
 */
template<typename T,
         typename Hash = std::hash<T>,
         typename KeyEqual = std::equal_to<T>>
class ThreadSafeCounter {
public:
    using key_type = T;
    using mapped_type = int;
    using value_type = std::pair<const T, int>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using hasher = Hash;
    using key_equal = KeyEqual;
    // Note: Direct reference/pointer to internal map elements is unsafe for concurrent access.
    // Methods will return copies or values instead.

private:
    using container_type = std::unordered_map<T, int, Hash, KeyEqual>;
    container_type counts_;
    mutable std::mutex mutex_; // Mutex to protect counts_

public:
    // Constructors
    ThreadSafeCounter() = default;

    explicit ThreadSafeCounter(size_type bucket_count,
                               const Hash& hash = Hash{},
                               const KeyEqual& equal = KeyEqual{})
        : counts_(bucket_count, hash, equal) {}

    template <typename SfinaeDummy = void>
    ThreadSafeCounter(std::initializer_list<T> init,
                      typename std::enable_if_t<std::is_void_v<SfinaeDummy> && !is_std_pair_v<T>>* = nullptr) {
        // No lock needed here as it's a constructor, object not yet shared.
        counts_.reserve(init.size());
        for (const auto& val : init) {
            counts_[val]++;
        }
    }

    template<typename InputIt>
    ThreadSafeCounter(InputIt first, InputIt last) {
        // No lock needed here as it's a constructor.
        if constexpr (std::is_base_of_v<std::forward_iterator_tag,
                      typename std::iterator_traits<InputIt>::iterator_category>) {
            counts_.reserve(std::distance(first, last));
        }
        for (auto it = first; it != last; ++it) {
            counts_[*it]++;
        }
    }

    ThreadSafeCounter(std::initializer_list<std::pair<T, int>> init) {
        // No lock needed here as it's a constructor.
        counts_.reserve(init.size());
        for (const auto& [key, count] : init) {
            if (count > 0) {
                counts_[key] = count;
            }
        }
    }

    // Copy constructor: needs to lock the source's mutex
    ThreadSafeCounter(const ThreadSafeCounter& other) {
        std::lock_guard<std::mutex> lock(other.mutex_);
        counts_ = other.counts_;
    }

    // Move constructor: needs to lock the source's mutex
    // (though technically moving from an rvalue should be safe if source is not used after move)
    ThreadSafeCounter(ThreadSafeCounter&& other) noexcept {
        std::lock_guard<std::mutex> lock(other.mutex_);
        counts_ = std::move(other.counts_);
    }

    // Copy assignment: needs to lock both mutexes (or use a lock manager for deadlock safety)
    ThreadSafeCounter& operator=(const ThreadSafeCounter& other) {
        if (this == &other) {
            return *this;
        }
        std::unique_lock<std::mutex> lock_this(mutex_, std::defer_lock);
        std::unique_lock<std::mutex> lock_other(other.mutex_, std::defer_lock);
        std::lock(lock_this, lock_other); // Deadlock avoidance
        counts_ = other.counts_;
        return *this;
    }

    // Move assignment: needs to lock both mutexes
    ThreadSafeCounter& operator=(ThreadSafeCounter&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        std::unique_lock<std::mutex> lock_this(mutex_, std::defer_lock);
        std::unique_lock<std::mutex> lock_other(other.mutex_, std::defer_lock);
        std::lock(lock_this, lock_other); // Deadlock avoidance
        counts_ = std::move(other.counts_);
        // Clear the source map after move under its lock
        other.counts_.clear();
        return *this;
    }

    // Core operations
    void add(const T& value, int count_val = 1) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (count_val > 0) {
            counts_[value] += count_val;
        } else if (count_val < 0) {
            _subtract_nolock(value, -count_val);
        }
    }

    void add(T&& value, int count_val = 1) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (count_val > 0) {
            counts_[std::move(value)] += count_val;
        } else if (count_val < 0) {
            // Need to be careful with move if key already exists.
            // Find first, then decide. For simplicity, let's just use the const T& version for subtraction.
            // This could be optimized if profiling showed it as a bottleneck.
            _subtract_nolock(value, -count_val);
        }
    }

    void subtract(const T& value, int count_val = 1) {
        std::lock_guard<std::mutex> lock(mutex_);
        _subtract_nolock(value, count_val);
    }

    // Internal subtract without locking, assumes lock is held
private:
    void _subtract_nolock(const T& value, int count_val) {
        if (count_val == 0) return; // Subtracting 0 changes nothing.
                                    // Note: count_val here is the amount to decrement by.
                                    // So if original call was subtract(key, -5), count_val here is -5.
                                    // This means we are effectively adding 5.
                                    // Python's Counter.subtract({'a': -5}) means c['a'] += 5.
                                    // So, if subtract(key, X) is called, we modify by -X.
                                    // Let's assume count_val in _subtract_nolock is the actual value from `other` counter or amount.
                                    // The public subtract method: subtract(value, amount_to_subtract)
                                    // calls _subtract_nolock(value, amount_to_subtract)
                                    // The public add method: add(value, amount_to_add)
                                    // if amount_to_add < 0, calls _subtract_nolock(value, -amount_to_add)

        // To align with Python's Counter.subtract behavior (which can create negative counts):
        // counts_[value] -= amount_to_subtract;
        // This means if 'value' is not present, it's default-initialized to 0, then subtracted.
        // So, if not present, counts_[value] becomes -amount_to_subtract.

        // Let's make count_val always positive for the logic of decrementing
        // No, the current interface of _subtract_nolock is: subtract 'count_val' (if positive) from the item.
        // The `add` method calls it with `_subtract_nolock(value, -negative_add_amount)`, so `count_val` becomes positive.
        // The `subtract` method calls it with `_subtract_nolock(value, positive_subtract_amount)`.
        // So `count_val` in `_subtract_nolock` is effectively always the positive magnitude to decrement by.

        // Original _subtract_nolock logic:
        // if (count_val <= 0) return; // count_val is magnitude to subtract
        // auto it = counts_.find(value);
        // if (it != counts_.end()) {
        //    it->second -= count_val;
        //    if (it->second <= 0) {
        //        counts_.erase(it); // THIS IS THE LINE TO CHANGE BEHAVIOR
        //    }
        // }
        // This means if item not found, nothing happens. Python's subtract would make it -count_val.

        // New logic for _subtract_nolock to align with Python's Counter.subtract():
        // Here, `count_val` is the (positive) amount by which to decrement the count of `value`.
        if (count_val <= 0) return; // Guard: only subtract positive magnitudes.

        auto it = counts_.find(value);
        if (it != counts_.end()) {
            it->second -= count_val;
            // No erase: allow counts to become zero or negative.
        } else {
            // If value was not present, subtracting count_val makes its count -count_val.
            counts_[value] = -count_val;
        }
    }
public:

    [[nodiscard]] int count(const T& value) const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (const auto it = counts_.find(value); it != counts_.end()) {
            return it->second;
        }
        return 0;
    }

    // operator[] for reading: same as count()
    [[nodiscard]] int operator[](const T& value) const {
        return count(value);
    }

    // Note: A non-const operator[] that returns a reference `int&` is problematic for thread safety
    // because the lock would be released before the caller modifies the value.
    // Instead, provide explicit add/subtract methods, or a method that takes a functor to modify the value.
    // For simplicity, we'll stick to add/subtract and count.
    // If direct modification like map[key] = val is desired, it should be a separate method
    // that takes key and value, e.g., set_count(key, val).

    void set_count(const T& key, int val) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (val > 0) {
            counts_[key] = val;
        } else {
            counts_.erase(key); // Remove if count is zero or negative
        }
    }


    [[nodiscard]] bool contains(const T& value) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = counts_.find(value);
        if (it != counts_.end()) {
            return it->second > 0; // Mimic Python's Counter: only true if count > 0
        }
        return false; // Not found or count <= 0
    }

    size_type erase(const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        return counts_.erase(value);
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        counts_.clear();
    }

    [[nodiscard]] size_type size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        // Mimic Python's len(Counter), which counts items with positive counts.
        size_type positive_count_size = 0;
        for (const auto& pair : counts_) {
            if (pair.second > 0) {
                positive_count_size++;
            }
        }
        return positive_count_size;
    }

    [[nodiscard]] bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return counts_.empty();
    }

    // Iterator support is tricky for thread-safe containers if iterators can be invalidated.
    // A common approach is to return a copy of the data or provide a way to iterate under lock.
    // For simplicity, `most_common` and `get_data_copy` return copies.
    // Direct begin()/end() returning iterators to the internal map are omitted for safety.

    [[nodiscard]] std::vector<std::pair<T, int>> most_common(size_type n = 0) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::pair<T, int>> items;
        items.reserve(counts_.size());

        for (const auto& pair : counts_) {
            items.push_back(pair); // pair is const&, so key is copied, int is copied
        }

        if (n > 0 && n < items.size()) {
            std::partial_sort(items.begin(), items.begin() + n, items.end(),
                            [](const auto& a, const auto& b) {
                                if (a.second != b.second) {
                                    return a.second > b.second;
                                }
                                // Consistent tie-breaking if T is comparable
                                if constexpr (is_lt_comparable_v<T>) {
                                   return a.first < b.first;
                                }
                                return false; // Or some other consistent tie-breaking
                            });
            items.resize(n);
        } else {
            std::sort(items.begin(), items.end(),
                     [](const auto& a, const auto& b) {
                         if (a.second != b.second) {
                             return a.second > b.second;
                         }
                         if constexpr (is_lt_comparable_v<T>) {
                            return a.first < b.first;
                         }
                         return false;
                     });
        }
        return items;
    }

    // Arithmetic operations: these create a new counter or modify `this` counter.
    // For `this` modification, lock is internal. For returning new, they operate on copies.

    ThreadSafeCounter& operator+=(const ThreadSafeCounter& other) {
        std::unique_lock<std::mutex> lock_this(mutex_, std::defer_lock);
        std::unique_lock<std::mutex> lock_other(other.mutex_, std::defer_lock);
        std::lock(lock_this, lock_other);

        for (const auto& [key, count_val] : other.counts_) {
            counts_[key] += count_val;
        }
        return *this;
    }

    ThreadSafeCounter& operator-=(const ThreadSafeCounter& other) {
        std::unique_lock<std::mutex> lock_this(mutex_, std::defer_lock);
        std::unique_lock<std::mutex> lock_other(other.mutex_, std::defer_lock);
        std::lock(lock_this, lock_other);

        for (const auto& [key, count_val] : other.counts_) {
            _subtract_nolock(key, count_val); // Use internal subtract that assumes lock
        }
        return *this;
    }

    // These return by value, so they create a temporary.
    // The temporary's operations are on copies of data from `this` and `other`.
    [[nodiscard]] ThreadSafeCounter operator+(const ThreadSafeCounter& other) const {
        ThreadSafeCounter result; // Default constructor
        {
            std::unique_lock<std::mutex> lock_this(mutex_, std::defer_lock);
            std::unique_lock<std::mutex> lock_other(other.mutex_, std::defer_lock);
            std::lock(lock_this, lock_other);
            result.counts_ = this->counts_; // Copy our data
            for (const auto& [key, count_val] : other.counts_) {
                result.counts_[key] += count_val;
            }
        }
        return result;
    }

    [[nodiscard]] ThreadSafeCounter operator-(const ThreadSafeCounter& other) const {
        ThreadSafeCounter result;
        {
            std::unique_lock<std::mutex> lock_this(mutex_, std::defer_lock);
            std::unique_lock<std::mutex> lock_other(other.mutex_, std::defer_lock);
            std::lock(lock_this, lock_other);
            result.counts_ = this->counts_; // Start with a copy of this counter's data

            for (const auto& pair_in_other : other.counts_) {
                const T& key = pair_in_other.first;
                int val_in_other = pair_in_other.second;

                if (val_in_other == 0) {
                    // If key is not in result, and we subtract 0, it remains not in result.
                    // If key is in result, and we subtract 0, its count is unchanged.
                    // So, optimize by skipping if other's count for this key is 0.
                    continue;
                }

                auto it = result.counts_.find(key);
                if (it != result.counts_.end()) { // Key was in `this` (and thus in `result`)
                    it->second -= val_in_other;
                } else { // Key was not in `this`, so effectively 0 - val_in_other
                    result.counts_[key] = -val_in_other;
                }
            }
        }
        return result;
    }

    // Comparison operators
    [[nodiscard]] bool operator==(const ThreadSafeCounter& other) const {
        std::unique_lock<std::mutex> lock_this(mutex_, std::defer_lock);
        std::unique_lock<std::mutex> lock_other(other.mutex_, std::defer_lock);
        std::lock(lock_this, lock_other);
        return counts_ == other.counts_;
    }

    [[nodiscard]] bool operator!=(const ThreadSafeCounter& other) const {
        // This will call operator==, which handles locking.
        return !(*this == other);
    }

    // Utility methods
    [[nodiscard]] int total() const {
        std::lock_guard<std::mutex> lock(mutex_);
        int sum = 0;
        for (const auto& [key, count_val] : counts_) {
            sum += count_val;
        }
        return sum;
    }

    // Method to get a copy of the internal data (thread-safe)
    [[nodiscard]] std::unordered_map<T, int, Hash, KeyEqual> get_data_copy() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return counts_;
    }

    // For operations like reserve, rehash, load_factor, etc., that affect the underlying unordered_map
    // These are less common for a counter but can be exposed if needed.
    void reserve(size_type n) {
        std::lock_guard<std::mutex> lock(mutex_);
        counts_.reserve(n);
    }

    [[nodiscard]] size_type bucket_count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return counts_.bucket_count();
    }

    [[nodiscard]] float load_factor() const { // unordered_map uses float
        std::lock_guard<std::mutex> lock(mutex_);
        return counts_.load_factor();
    }

    [[nodiscard]] float max_load_factor() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return counts_.max_load_factor();
    }

    void max_load_factor(float ml) {
        std::lock_guard<std::mutex> lock(mutex_);
        counts_.max_load_factor(ml);
    }

    void rehash(size_type n) {
        std::lock_guard<std::mutex> lock(mutex_);
        counts_.rehash(n);
    }

    // Set operations similar to Python's Counter
    // These operations return new ThreadSafeCounter objects.
    [[nodiscard]] ThreadSafeCounter intersection(const ThreadSafeCounter& other) const {
        ThreadSafeCounter result;
        std::unique_lock<std::mutex> lock_this(mutex_, std::defer_lock);
        std::unique_lock<std::mutex> lock_other(other.mutex_, std::defer_lock);
        std::lock(lock_this, lock_other);

        for (const auto& [key, count_val] : this->counts_) {
            if (auto it = other.counts_.find(key); it != other.counts_.end()) {
                result.counts_[key] = std::min(count_val, it->second);
            }
        }
        return result;
    }

    [[nodiscard]] ThreadSafeCounter union_with(const ThreadSafeCounter& other) const { // elements() equivalent
        ThreadSafeCounter result;
        std::unique_lock<std::mutex> lock_this(mutex_, std::defer_lock);
        std::unique_lock<std::mutex> lock_other(other.mutex_, std::defer_lock);
        std::lock(lock_this, lock_other);

        result.counts_ = this->counts_; // Start with a copy of this
        for (const auto& [key, count_val] : other.counts_) {
            auto it = result.counts_.find(key);
            if (it != result.counts_.end()) {
                it->second = std::max(it->second, count_val);
            } else {
                result.counts_[key] = count_val;
            }
        }
        return result;
    }
};

// Deduction guides (similar to Counter, adjusted for ThreadSafeCounter)
template<typename InputIt>
ThreadSafeCounter(InputIt, InputIt) -> ThreadSafeCounter<typename std::iterator_traits<InputIt>::value_type>;

template<
    typename T_Actual,
    typename SfinaeDummy = void
>
ThreadSafeCounter(
    std::initializer_list<T_Actual> init,
    typename std::enable_if_t<
        std::is_void_v<SfinaeDummy> &&
        !is_std_pair_v<T_Actual>
    >* = nullptr
) -> ThreadSafeCounter<T_Actual>;

template<typename T_Param> // Renamed T to T_Param to avoid conflict with class T
ThreadSafeCounter(std::initializer_list<std::pair<T_Param, int>>) -> ThreadSafeCounter<T_Param>;

// #pragma once // Redundant, but okay. // Moved is_lt_comparable_v to the top
