#pragma once

#include <array>
#include <initializer_list>
#include <type_traits>
#include <utility>
#include <stdexcept>

template <typename TEnum, typename TValue, std::size_t N = static_cast<std::size_t>(TEnum::COUNT)>
class EnumMap {
    static_assert(std::is_enum<TEnum>::value, "EnumMap requires an enum type");
    
private:
    std::array<TValue, N> data_;
    
    // Helper to validate enum is within bounds
    static constexpr std::size_t to_index(TEnum e) noexcept {
        return static_cast<std::size_t>(e);
    }
    
    static constexpr bool is_valid_enum(TEnum e) noexcept {
        return to_index(e) < N;
    }

public:
    // Type aliases for STL compatibility
    using key_type = TEnum;
    using mapped_type = TValue;
    using value_type = TValue;
    using size_type = std::size_t;
    using value_iterator = typename std::array<TValue, N>::iterator;
    using const_value_iterator = typename std::array<TValue, N>::const_iterator;
    using reference = TValue&;
    using const_reference = const TValue&;

    // Forward declarations for custom iterators
    template <bool IsConst> class EnumMapIterator;
    using iterator = EnumMapIterator<false>;
    using const_iterator = EnumMapIterator<true>;

    // Default constructor - default-initializes all values
    EnumMap() {
        if constexpr (std::is_default_constructible_v<TValue>) {
            // This will value-initialize if TValue is a POD type (e.g., int to 0)
            // or default-construct if TValue is a class type.
            for (size_type i = 0; i < N; ++i) {
                 data_[i] = TValue{};
            }
        }
        // If TValue is not default constructible, the behavior of data_ is to leave elements
        // uninitialized for non-class types, or call default constructors for class types
        // if std::array does so. However, our clear() and erase() will require default constructibility.
    }
    
    // Initializer list constructor
    // Delegates to the default constructor to ensure all elements are initialized,
    // then overwrites with values from the initializer list.
    EnumMap(std::initializer_list<std::pair<TEnum, TValue>> init) : EnumMap() {
        for (const auto& pair : init) {
            // Using data_ directly might be slightly safer if operator[] had side effects
            // or if TEnum was not perfectly mapping to indices (though our design ensures it).
            data_[to_index(pair.first)] = pair.second;
        }
    }
    
    // Copy constructor
    EnumMap(const EnumMap& other) = default;
    
    // Move constructor
    EnumMap(EnumMap&& other) noexcept = default;
    
    // Copy assignment
    EnumMap& operator=(const EnumMap& other) = default;
    
    // Move assignment
    EnumMap& operator=(EnumMap&& other) noexcept = default;
    
    // Destructor
    ~EnumMap() = default;

    // Element access - non-const version
    TValue& operator[](TEnum key) {
        // Consider adding a bounds check here if desired, or rely on at() for checked access.
        // For performance, operator[] often omits bounds checks.
        return data_[to_index(key)];
    }
    
    // Element access - const version
    const TValue& operator[](TEnum key) const {
        return data_[to_index(key)];
    }
    
    // Bounds-checked access - non-const version
    TValue& at(TEnum key) {
        if (!is_valid_enum(key)) {
            throw std::out_of_range("EnumMap::at: enum value out of range");
        }
        return data_[to_index(key)];
    }
    
    // Bounds-checked access - const version
    const TValue& at(TEnum key) const {
        if (!is_valid_enum(key)) {
            throw std::out_of_range("EnumMap::at: enum value out of range");
        }
        return data_[to_index(key)];
    }

    // --- Custom Key-Value Iterators ---
public:
    template <bool IsConst>
    class EnumMapIterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        // Value type is std::pair<const TEnum, maybe_const TValue>
        // Key is const because it shouldn't be changed via iterator.
        using value_type = std::pair<const TEnum, typename std::conditional_t<IsConst, const TValue, TValue>>;
        using difference_type = std::ptrdiff_t;

        // The pointer type for operator->()
        // For non-const iterators, we need a proxy object if TValue is not a reference.
        // However, since data_ stores TValue directly, operator* returns a reference to TValue (or const TValue).
        // So, the pair will be std::pair<const TEnum, TValue&> or std::pair<const TEnum, const TValue&>.
        // operator-> must return a pointer to such a pair. Since we construct it on the fly,
        // we need a helper struct or store the pair.
        // Let's simplify: operator* returns by value for the pair.
        // This is common for map-like iterators where key is by value and value is by reference.
        // For operator->, we can return a pointer to a temporary pair if needed, or disallow it if too complex.
        // Or, more simply, make value_type std::pair<TEnum, TValue*> or similar and adjust.
        // Let's stick to a common pattern: operator* returns a "proxy" or a pair of references.
        // std::map::iterator::value_type is std::pair<const Key, T>
        // operator* returns std::pair<const Key, T>&
        // This is hard because our TEnum is not stored, it's derived from index.

        // Let's define what operator* returns more directly.
        // It should be something like: struct { TEnum first; TValue& second; };
        // Or std::pair<TEnum, std::reference_wrapper<TValue>>
        // For simplicity, let's make operator* return a std::pair<TEnum, TValue&> (or const version)
        // This means value_type should reflect this.
        // value_type for iterator: std::pair<TEnum, TValue> - no, this implies copying.
        // value_type for iterator: not directly used by users usually.
        // reference for iterator: std::pair<TEnum, TValue&>
        // pointer for iterator: pointer to std::pair<TEnum, TValue&>

        // Let's try to make it similar to how std::map iterators work, but adapted.
        // The "value" iterated over is a conceptual pair.
        // operator* returns a reference to this conceptual pair.
        // Since TEnum is generated from the index, it cannot be part of the stored data.
        // So, we'll need a proxy reference type.

        struct PairProxy {
            TEnum first;
            typename std::conditional_t<IsConst, const TValue&, TValue&> second;

            // Allow conversion to std::pair for convenience, if needed
            operator std::pair<TEnum, typename std::conditional_t<IsConst, const TValue&, TValue&>>() const {
                return {first, second};
            }
            // operator-> on the proxy to access members of TValue directly if TValue is a struct/class
            typename std::conditional_t<IsConst, const TValue*, TValue*> operator->() {
                 return &second;
            }
        };
         struct ConstPairProxy {
            TEnum first;
            const TValue& second;

            operator std::pair<TEnum, const TValue&>() const {
                return {first, second};
            }
             const TValue* operator->() const { // Note: returns TValue* not PairProxy*
                 return &second;
            }
        };


        using reference = typename std::conditional_t<IsConst, ConstPairProxy, PairProxy>;
        // Pointer type is tricky for proxy objects. Often a proxy pointer or just raw pointer to TValue.
        // For now, let's use a simple pointer to the proxy object for illustration,
        // though this requires the proxy to be addressable or implement operator-> itself.
        // A common pattern is to return the proxy itself from operator-> if it has an operator*
        // Let's make operator-> return our proxy by value, which is unusual but works if proxy has ->
         using pointer = reference; // operator-> will return reference, which has its own ->


    private:
        friend class EnumMap<TEnum, TValue, N>; // Allow EnumMap to access private members
        using ArrayType = typename std::conditional_t<IsConst, const std::array<TValue, N>, std::array<TValue, N>>;

        ArrayType* data_ptr_ = nullptr;
        size_type current_index_ = 0;

        EnumMapIterator(ArrayType* data_ptr, size_type index)
            : data_ptr_(data_ptr), current_index_(index) {}

    public:
        EnumMapIterator() = default; // For default construction, e.g. as end iterator

        // Conversion from non-const to const iterator
        operator EnumMapIterator<true>() const {
            return EnumMapIterator<true>(data_ptr_, current_index_);
        }

        reference operator*() const {
            return {static_cast<TEnum>(current_index_), (*data_ptr_)[current_index_]};
        }

        pointer operator->() const {
            return operator*(); // Returns the proxy, which has its own operator->
        }

        EnumMapIterator& operator++() { // Pre-increment
            ++current_index_;
            return *this;
        }

        EnumMapIterator operator++(int) { // Post-increment
            EnumMapIterator temp = *this;
            ++(*this);
            return temp;
        }

        EnumMapIterator& operator--() { // Pre-decrement
            --current_index_;
            return *this;
        }

        EnumMapIterator operator--(int) { // Post-decrement
            EnumMapIterator temp = *this;
            --(*this);
            return temp;
        }

        EnumMapIterator& operator+=(difference_type offset) {
            current_index_ += offset;
            return *this;
        }

        EnumMapIterator operator+(difference_type offset) const {
            EnumMapIterator temp = *this;
            temp += offset;
            return temp;
        }

        EnumMapIterator& operator-=(difference_type offset) {
            current_index_ -= offset;
            return *this;
        }

        EnumMapIterator operator-(difference_type offset) const {
            EnumMapIterator temp = *this;
            temp -= offset;
            return temp;
        }

        difference_type operator-(const EnumMapIterator& other) const {
            return static_cast<difference_type>(current_index_) - static_cast<difference_type>(other.current_index_);
        }

        // operator[] - access element by offset
        reference operator[](difference_type offset) const {
            return *(*this + offset);
        }

        bool operator==(const EnumMapIterator& other) const {
            return data_ptr_ == other.data_ptr_ && current_index_ == other.current_index_;
        }

        bool operator!=(const EnumMapIterator& other) const {
            return !(*this == other);
        }

        bool operator<(const EnumMapIterator& other) const {
            // Add check for same container if necessary: (data_ptr_ == other.data_ptr_)
            return current_index_ < other.current_index_;
        }

        bool operator>(const EnumMapIterator& other) const {
            return other < *this;
        }

        bool operator<=(const EnumMapIterator& other) const {
            return !(*this > other);
        }

        bool operator>=(const EnumMapIterator& other) const {
            return !(*this < other);
        }
    };

    // Key-value iterators
    iterator begin() noexcept {
        return iterator(const_cast<std::array<TValue, N>*>(&data_), 0);
    }
    
    const_iterator begin() const noexcept {
        return const_iterator(const_cast<std::array<TValue, N>*>(&data_), 0);
    }
    
    const_iterator cbegin() const noexcept {
        return const_iterator(const_cast<std::array<TValue, N>*>(&data_), 0);
    }
    
    iterator end() noexcept {
        return iterator(const_cast<std::array<TValue, N>*>(&data_), N);
    }
    
    const_iterator end() const noexcept {
        return const_iterator(const_cast<std::array<TValue, N>*>(&data_), N);
    }
    
    const_iterator cend() const noexcept {
        return const_iterator(const_cast<std::array<TValue, N>*>(&data_), N);
    }

    // Value-only iterators (renamed for clarity)
    value_iterator value_begin() noexcept {
        return data_.begin();
    }

    const_value_iterator value_begin() const noexcept {
        return data_.begin();
    }

    const_value_iterator const_value_cbegin() const noexcept {
        return data_.cbegin();
    }

    value_iterator value_end() noexcept {
        return data_.end();
    }

    const_value_iterator value_end() const noexcept {
        return data_.end();
    }

    const_value_iterator const_value_cend() const noexcept {
        return data_.cend();
    }


    // Capacity
    constexpr std::size_t size() const noexcept {
        return N;
    }
    
    constexpr bool empty() const noexcept {
        return N == 0;
    }
    
    constexpr std::size_t max_size() const noexcept {
        return N;
    }

    // Lookup
    bool contains(TEnum key) const noexcept {
        return is_valid_enum(key);
    }

    // Modifiers
    void fill(const TValue& value) {
        data_.fill(value);
    }

    void clear() noexcept(std::is_nothrow_default_constructible_v<TValue> && std::is_nothrow_assignable_v<TValue&, TValue&&>) {
        static_assert(std::is_default_constructible_v<TValue>, "TValue must be default constructible to use clear().");
        for (size_type i = 0; i < N; ++i) {
            data_[i] = TValue{}; // Default construct and assign
        }
    }

    // Erases an element by resetting it to a default-constructed value.
    // Returns true if key was valid and element was "erased", false otherwise.
    bool erase(TEnum key) noexcept(std::is_nothrow_default_constructible_v<TValue> && std::is_nothrow_assignable_v<TValue&, TValue&&>) {
        static_assert(std::is_default_constructible_v<TValue>, "TValue must be default constructible to use erase().");
        if (!is_valid_enum(key)) {
            return false;
        }
        data_[to_index(key)] = TValue{};
        return true;
    }
    
    void swap(EnumMap& other) noexcept {
        data_.swap(other.data_);
    }

    // Direct access to underlying array (for advanced use cases)
    std::array<TValue, N>& data() noexcept {
        return data_;
    }
    
    const std::array<TValue, N>& data() const noexcept {
        return data_;
    }

    // Equality comparison
    bool operator==(const EnumMap& other) const {
        return data_ == other.data_;
    }
    
    bool operator!=(const EnumMap& other) const {
        return data_ != other.data_;
    }
};

// Deduction guide for C++17 (helps with template argument deduction)
template<typename TEnum, typename TValue>
EnumMap(std::initializer_list<std::pair<TEnum, TValue>>) 
    -> EnumMap<TEnum, TValue, static_cast<std::size_t>(TEnum::COUNT)>;

// Convenience function for swapping
template<typename TEnum, typename TValue, std::size_t N>
void swap(EnumMap<TEnum, TValue, N>& lhs, EnumMap<TEnum, TValue, N>& rhs) noexcept {
    lhs.swap(rhs);
}

// Removed example code from here. It's now in examples/enum_map_example.cpp
