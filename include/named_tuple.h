#pragma once

#include <iostream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace cpp_utils {

// --- Implementation Details ---

namespace detail {

// Helper to find the index of a type T in a pack of types Ts...
template <typename T, typename... Ts>
struct type_index;

// Base case: T is the head of the pack, index is 0.
template <typename T, typename... Ts>
struct type_index<T, T, Ts...> : std::integral_constant<std::size_t, 0> {};

// Recursive step: T is not the head, add 1 to the index of the rest of the pack.
template <typename T, typename U, typename... Ts>
struct type_index<T, U, Ts...> : std::integral_constant<std::size_t, 1 + type_index<T, Ts...>::value> {};

// Helper variable template to get the index value.
template <typename T, typename... Ts>
constexpr std::size_t type_index_v = type_index<T, Ts...>::value;

} // namespace detail

// --- Main named_tuple class ---

/**
 * @brief A C++17 equivalent of Python's namedtuple.
 *
 * @tparam IsMutable If true, the tuple's fields can be modified after creation.
 * @tparam Fields A parameter pack of "field" structs. Each struct must define a `type` alias.
 */
template <bool IsMutable, typename... Fields>
class named_tuple {
public:
    // The underlying std::tuple that stores the data.
    // The types are extracted from the `type` alias within each Field struct.
    using value_tuple = std::tuple<typename Fields::type...>;

    /**
     * @brief Default constructor.
     */
    named_tuple() = default;

    /**
     * @brief Constructor that takes arguments to initialize the fields.
     *
     * @param args The values to initialize the tuple fields with.
     */
    template <typename... Args>
    explicit named_tuple(Args&&... args) : data_(std::forward<Args>(args)...) {}

    /**
     * @brief Access a field by its compile-time index.
     * This is the const version, for both mutable and immutable tuples.
     * @tparam I The index of the field.
     * @return A const reference to the field's value.
     */
    template <std::size_t I>
    const auto& get() const {
        return std::get<I>(data_);
    }

    /**
     * @brief Access a field by its compile-time index for modification.
     * This version is only enabled if the named_tuple is mutable.
     * @tparam I The index of the field.
     * @return A non-const reference to the field's value.
     */
    template <std::size_t I>
    auto& get() {
        static_assert(IsMutable, "Cannot call non-const get() on an immutable named_tuple. Make it mutable to allow modification.");
        return std::get<I>(data_);
    }

    /**
     * @brief Access a field by its "field type".
     * This is the const version, for both mutable and immutable tuples.
     * @tparam Field The field type (the struct you defined).
     * @return A const reference to the field's value.
     */
    template <typename Field>
    const auto& get() const {
        constexpr std::size_t index = detail::type_index_v<Field, Fields...>;
        return std::get<index>(data_);
    }

    /**
     * @brief Access a field by its "field type" for modification.
     * This version is only enabled if the named_tuple is mutable.
     * @tparam Field The field type (the struct you defined).
     * @return A non-const reference to the field's value.
     */
    template <typename Field>
    auto& get() {
        static_assert(IsMutable, "Cannot call non-const get() on an immutable named_tuple. Make it mutable to allow modification.");
        constexpr std::size_t index = detail::type_index_v<Field, Fields...>;
        return std::get<index>(data_);
    }

    /**
     * @brief Returns the number of fields in the tuple.
     */
    constexpr std::size_t size() const {
        return sizeof...(Fields);
    }

    /**
     * @brief Get the underlying std::tuple.
     */
    const value_tuple& to_tuple() const {
        return data_;
    }

private:
    value_tuple data_;
};

/**
 * @brief A helper macro to simplify the definition of named_tuple fields.
 *
 * @param name The name of the field struct.
 * @param type_name The underlying C++ type for the field.
 */
#define DEFINE_NAMED_TUPLE_FIELD(name, type_name) \
    struct name { using type = type_name; }

} // namespace cpp_utils
