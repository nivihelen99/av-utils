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
template <typename... ViewTypes> // These are the types stored in the view's tuple (T or T&)
class ZippedView;

// Iterator implementation
template <typename... ContTypes> // These are the types of the containers being iterated (T or T&)
class ZippedIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    // reference_t<ContTypes> will correctly give U& if ContTypes is U or U&
    using value_type = std::tuple<reference_t<ContTypes>...>;
    using difference_type = std::ptrdiff_t;
    using pointer = void; // Pointers to tuples are not common for iterators
    using reference = value_type; // operator* returns by value (a tuple of references)

private:
    // iterator_t<ContTypes> will correctly give iterator type for U or U&
    std::tuple<iterator_t<ContTypes>...> iterators_;

public:
    explicit ZippedIterator(iterator_t<ContTypes>... iters)
        : iterators_(std::move(iters)...) {}

    auto operator*() const -> value_type {
        return dereference_impl(std::index_sequence_for<ContTypes...>{});
    }

    ZippedIterator& operator++() {
        increment_impl(std::index_sequence_for<ContTypes...>{});
        return *this;
    }

    ZippedIterator operator++(int) {
        auto copy = *this;
        ++(*this);
        return copy;
    }

    bool operator==(const ZippedIterator& other) const {
        return equals_impl(other, std::index_sequence_for<ContTypes...>{});
    }

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
        (void)(++std::get<Is>(iterators_), ...); // Fold expression
    }

    template <std::size_t... Is>
    bool equals_impl(const ZippedIterator& other, std::index_sequence<Is...>) const {
        // Two zip iterators are equal if *any* of their underlying iterators are equal.
        // This implies that the shortest sequence has been exhausted when comparing to an end iterator.
        bool result = false;
        ((result = result || (std::get<Is>(iterators_) == std::get<Is>(other.iterators_))), ...);
        return result;
    }
};

// Const iterator implementation
template <typename... ContTypes> // These are types of containers (T or T&)
class ZippedConstIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::tuple<const_reference_t<ContTypes>...>;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = value_type;

private:
    std::tuple<const_iterator_t<ContTypes>...> iterators_;

public:
    explicit ZippedConstIterator(const_iterator_t<ContTypes>... iters)
        : iterators_(std::move(iters)...) {}

    auto operator*() const -> value_type {
        return dereference_impl(std::index_sequence_for<ContTypes...>{});
    }

    ZippedConstIterator& operator++() {
        increment_impl(std::index_sequence_for<ContTypes...>{});
        return *this;
    }

    ZippedConstIterator operator++(int) {
        auto copy = *this;
        ++(*this);
        return copy;
    }

    bool operator==(const ZippedConstIterator& other) const {
        return equals_impl(other, std::index_sequence_for<ContTypes...>{});
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
        (void)(++std::get<Is>(iterators_), ...);
    }

    template <std::size_t... Is>
    bool equals_impl(const ZippedConstIterator& other, std::index_sequence<Is...>) const {
        bool result = false;
        ((result = result || (std::get<Is>(iterators_) == std::get<Is>(other.iterators_))), ...);
        return result;
    }
};


// Main ZippedView class
// ViewTypes are the types passed to the factory (e.g., std::vector<int>&, std::list<std::string>&&)
// The tuple will store these as is (e.g. T& for lvalues, T for rvalues if moved)
template <typename... ViewTypes>
class ZippedView {
public:
    // The iterators operate on these ViewTypes directly.
    // std::begin/end work on T and T&.
    using iterator = ZippedIterator<ViewTypes...>;
    using const_iterator = ZippedConstIterator<ViewTypes...>;

private:
    std::tuple<ViewTypes...> containers_tuple_; // Stores T& or T (if rvalue was moved in)

public:
    // Constructor takes forwarding references to perfectly forward into the tuple
    explicit ZippedView(ViewTypes&&... containers)
        : containers_tuple_(std::forward<ViewTypes>(containers)...) {}

    iterator begin() {
        return make_iterator<iterator>([](auto& c){ return std::begin(c); });
    }

    iterator end() {
        return make_iterator<iterator>([](auto& c){ return std::end(c); });
    }

    const_iterator begin() const {
        return make_iterator<const_iterator>([](const auto& c){ return std::begin(c); });
    }

    const_iterator end() const {
        return make_iterator<const_iterator>([](const auto& c){ return std::end(c); });
    }

    const_iterator cbegin() const {
        return begin();
    }

    const_iterator cend() const {
        return end();
    }

private:
    template <typename Iter, typename Func, std::size_t... Is>
    Iter make_iterator_impl(Func f, std::index_sequence<Is...>) {
        return Iter(f(std::get<Is>(containers_tuple_))...);
    }

    template <typename Iter, typename Func, std::size_t... Is>
    Iter make_iterator_impl_const(Func f, std::index_sequence<Is...>) const {
        return Iter(f(std::get<Is>(containers_tuple_))...);
    }

    template <typename Iter, typename Func>
    Iter make_iterator(Func f) {
        return make_iterator_impl<Iter>(f, std::index_sequence_for<ViewTypes...>{});
    }

    template <typename Iter, typename Func>
    Iter make_iterator(Func f) const {
        return make_iterator_impl_const<Iter>(f, std::index_sequence_for<ViewTypes...>{});
    }
};

// Factory function for zip
// Takes Containers&&... (e.g., T&, U&&)
// Returns ZippedView<T&, U> (if U was rvalue, it's moved into ZippedView)
template <typename... Containers>
auto zip(Containers&&... containers) -> ZippedView<Containers...> {
    return ZippedView<Containers...>(std::forward<Containers>(containers)...);
}

// Bonus: Enumerate implementation
// ViewType is T or T& (if an rvalue container is passed, ViewType is T, otherwise T&)
template <typename ViewType>
class EnumerateView {
public:
    // Use ViewType for iterator_t and reference_t
    using container_iterator_type = iterator_t<ViewType>;
    using container_reference_type = reference_t<ViewType>;
    // value_type for the view itself, though not strictly needed here
    using view_value_type = std::pair<std::size_t, container_reference_type>;


    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        // The iterator's value_type is a pair of index and reference to element
        using value_type = std::pair<std::size_t, container_reference_type>;
        using difference_type = std::ptrdiff_t;
        using pointer = void; // Not typical for such iterators
        using reference = value_type; // operator* returns by value (the pair)

    private:
        std::size_t index_;
        container_iterator_type iter_;
        // Store end iterator to determine when iteration should stop, especially if container is empty or for equality.
        // container_iterator_type end_iter_; // Optional: only if equality needs it explicitly

    public:
        iterator(std::size_t index, container_iterator_type iter) // Removed end_iter_ for simplicity for now
            : index_(index), iter_(iter) {}

        auto operator*() const -> value_type {
            return value_type(index_, *iter_); // Constructs pair (size_t, T&)
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

        // Equality comparison: only the underlying container iterator matters.
        // The index is auxiliary information for dereferencing.
        bool operator==(const iterator& other) const {
            return iter_ == other.iter_;
        }

        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }
    };

private:
    ViewType container_; // Stores T (if rvalue moved in) or T&

public:
    // Constructor takes a forwarding reference to store T or T&
    explicit EnumerateView(ViewType&& container)
        : container_(std::forward<ViewType>(container)) {}

    iterator begin() {
        return iterator(0, std::begin(container_));
    }

    iterator end() {
        // The index for the end iterator can be the size of the container.
        // This is mostly for conceptual correctness or if size() is implemented.
        // For typical range-based for loops, only iter_ equality matters.
        return iterator(std::distance(std::begin(container_), std::end(container_)), std::end(container_));
    }

    // TODO: Add const begin()/end() and cbegin()/cend() if const enumeration is desired.
    // This would involve a const_iterator for EnumerateView.
};

// Factory function for enumerate
// Container will be deduced as T& for lvalues, T for rvalues (after std::forward)
// So EnumerateView will be instantiated with EnumerateView<T&> or EnumerateView<T>
template <typename Container>
auto enumerate(Container&& container) -> EnumerateView<Container> {
    return EnumerateView<Container>(std::forward<Container>(container));
}

} // namespace zip_utils
