#ifndef AOSX_L2_RETURN_H
#define AOSX_L2_RETURN_H

#include <iostream>
#include <map>
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <functional>


namespace aos_utils
{

/**
 * @brief A class template representing an unexpected error value.
 * 
 * This class is used to encapsulate an error value of type `E`.
 * It provides various methods to access the stored error with different value categories.
 * 
 * @tparam E The type of the error value to be stored.
 */
template <typename E>
class Unexpected
{
public:
    // Template constructor for perfect forwarding
    template <typename U = E, 
              typename = std::enable_if_t<std::is_constructible_v<E, U> && 
                                        !std::is_same_v<std::decay_t<U>, Unexpected>>>
    explicit constexpr Unexpected(U&& error) noexcept(std::is_nothrow_constructible_v<E, U>)
        : m_error(std::forward<U>(error))
    {
    }

    // Reference access methods with proper const-correctness and noexcept
    constexpr const E& error() const& noexcept { return m_error; }
    constexpr E& error() & noexcept { return m_error; }
    constexpr E&& error() && noexcept { return std::move(m_error); }
    constexpr const E&& error() const&& noexcept { return std::move(m_error); }

    // Equality operators
    template <typename E2>
    constexpr bool operator==(const Unexpected<E2>& other) const noexcept(noexcept(m_error == other.error()))
    {
        return m_error == other.error();
    }

    template <typename E2>
    constexpr bool operator!=(const Unexpected<E2>& other) const noexcept(noexcept(!(*this == other)))
    {
        return !(*this == other);
    }

private:
    E m_error;
};

// Deduction guide for Unexpected
template <typename E>
Unexpected(E) -> Unexpected<E>;

/**
 * @brief A class template representing a value or an error.
 * 
 * The `Expected` class is used to encapsulate either a value of type `T` or an error of type `E`.
 * It provides methods to access the value or the error and supports monadic operations.
 * 
 * @tparam T The type of the value to be stored.
 * @tparam E The type of the error to be stored. Defaults to `std::string`.
 */
template <typename T, typename E = std::string>
class Expected
{
public:
    using value_type = T;
    using error_type = E;
    using unexpected_type = Unexpected<E>;

    // Default constructor (only available if T is default constructible)
    template <typename U = T, typename = std::enable_if_t<std::is_default_constructible_v<U>>>
    constexpr Expected() noexcept(std::is_nothrow_default_constructible_v<T>)
        : m_value_or_error(std::in_place_index<0>, T{})
    {
    }

    // Value constructors with perfect forwarding and SFINAE
    template <typename U = T,
              typename = std::enable_if_t<
                  std::is_constructible_v<T, U> &&
                  !std::is_same_v<std::decay_t<U>, Expected> &&
                  !std::is_same_v<std::decay_t<U>, Unexpected<E>>>>
    constexpr Expected(U&& value) noexcept(std::is_nothrow_constructible_v<T, U>)
        : m_value_or_error(std::in_place_index<0>, std::forward<U>(value))
    {
    }

    // Unexpected constructors
    template <typename G = E,
              typename = std::enable_if_t<std::is_constructible_v<E, const G&>>>
    constexpr Expected(const Unexpected<G>& unexpected) noexcept(std::is_nothrow_constructible_v<E, const G&>)
        : m_value_or_error(std::in_place_index<1>, unexpected.error())
    {
    }

    template <typename G = E,
              typename = std::enable_if_t<std::is_constructible_v<E, G>>>
    constexpr Expected(Unexpected<G>&& unexpected) noexcept(std::is_nothrow_constructible_v<E, G>)
        : m_value_or_error(std::in_place_index<1>, std::move(unexpected.error()))
    {
    }

    // In-place construction
    template <typename... Args,
              typename = std::enable_if_t<std::is_constructible_v<T, Args...>>>
    constexpr explicit Expected(std::in_place_t, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
        : m_value_or_error(std::in_place_index<0>, std::forward<Args>(args)...)
    {
    }

    template <typename... Args,
              typename = std::enable_if_t<std::is_constructible_v<E, Args...>>>
    constexpr explicit Expected(std::in_place_type_t<Unexpected<E>>, Args&&... args) noexcept(std::is_nothrow_constructible_v<E, Args...>)
        : m_value_or_error(std::in_place_index<1>, std::forward<Args>(args)...)
    {
    }

    // Copy and move constructors with proper noexcept specifications
    Expected(const Expected&) = default;
    Expected(Expected&&) noexcept(std::is_nothrow_move_constructible_v<T> && 
                                  std::is_nothrow_move_constructible_v<E>) = default;

    // Assignment operators with proper noexcept specifications
    Expected& operator=(const Expected&) = default;
    Expected& operator=(Expected&&) noexcept(std::is_nothrow_move_assignable_v<T> && 
                                            std::is_nothrow_move_assignable_v<E>) = default;

    // Value assignment
    template <typename U = T,
              typename = std::enable_if_t<
                  std::is_constructible_v<T, U> &&
                  std::is_assignable_v<T&, U> &&
                  !std::is_same_v<std::decay_t<U>, Expected> &&
                  !std::is_same_v<std::decay_t<U>, Unexpected<E>>>>
    Expected& operator=(U&& value) noexcept(std::is_nothrow_constructible_v<T, U> && 
                                           std::is_nothrow_assignable_v<T&, U>)
    {
        if (has_value()) {
            std::get<0>(m_value_or_error) = std::forward<U>(value);
        } else {
            m_value_or_error.template emplace<0>(std::forward<U>(value));
        }
        return *this;
    }

    // Unexpected assignment
    template <typename G>
    Expected& operator=(const Unexpected<G>& unexpected) noexcept(std::is_nothrow_constructible_v<E, const G&> && 
                                                                 std::is_nothrow_assignable_v<E&, const G&>)
    {
        if (has_value()) {
            m_value_or_error.template emplace<1>(unexpected.error());
        } else {
            std::get<1>(m_value_or_error) = unexpected.error();
        }
        return *this;
    }

    template <typename G>
    Expected& operator=(Unexpected<G>&& unexpected) noexcept(std::is_nothrow_constructible_v<E, G> && 
                                                            std::is_nothrow_assignable_v<E&, G>)
    {
        if (has_value()) {
            m_value_or_error.template emplace<1>(std::move(unexpected.error()));
        } else {
            std::get<1>(m_value_or_error) = std::move(unexpected.error());
        }
        return *this;
    }

    // Destructor
    ~Expected() = default;

    // Observer methods
    constexpr bool has_value() const noexcept
    {
        return m_value_or_error.index() == 0;
    }

    constexpr explicit operator bool() const noexcept
    {
        return has_value();
    }

    // Value access with proper exception handling
    constexpr const T& value() const&
    {
        if (!has_value()) [[unlikely]]
            throw std::bad_variant_access{};
        return std::get<0>(m_value_or_error);
    }

    constexpr T& value() &
    {
        if (!has_value()) [[unlikely]]
            throw std::bad_variant_access{};
        return std::get<0>(m_value_or_error);
    }

    constexpr T&& value() &&
    {
        if (!has_value()) [[unlikely]]
            throw std::bad_variant_access{};
        return std::move(std::get<0>(m_value_or_error));
    }

    constexpr const T&& value() const&&
    {
        if (!has_value()) [[unlikely]]
            throw std::bad_variant_access{};
        return std::move(std::get<0>(m_value_or_error));
    }

    // Error access
    constexpr const E& error() const&
    {
        if (has_value()) [[unlikely]]
            throw std::bad_variant_access{};
        return std::get<1>(m_value_or_error);
    }

    constexpr E& error() &
    {
        if (has_value()) [[unlikely]]
            throw std::bad_variant_access{};
        return std::get<1>(m_value_or_error);
    }

    constexpr E&& error() &&
    {
        if (has_value()) [[unlikely]]
            throw std::bad_variant_access{};
        return std::move(std::get<1>(m_value_or_error));
    }

    constexpr const E&& error() const&&
    {
        if (has_value()) [[unlikely]]
            throw std::bad_variant_access{};
        return std::move(std::get<1>(m_value_or_error));
    }

    // Unchecked access (for performance-critical code)
    constexpr const T& operator*() const& noexcept { return std::get<0>(m_value_or_error); }
    constexpr T& operator*() & noexcept { return std::get<0>(m_value_or_error); }
    constexpr T&& operator*() && noexcept { return std::move(std::get<0>(m_value_or_error)); }
    constexpr const T&& operator*() const&& noexcept { return std::move(std::get<0>(m_value_or_error)); }

    constexpr const T* operator->() const noexcept { return &std::get<0>(m_value_or_error); }
    constexpr T* operator->() noexcept { return &std::get<0>(m_value_or_error); }

    // Value or default
    template <typename U>
    constexpr T value_or(U&& default_value) const&
    {
        return has_value() ? **this : static_cast<T>(std::forward<U>(default_value));
    }

    template <typename U>
    constexpr T value_or(U&& default_value) &&
    {
        return has_value() ? std::move(**this) : static_cast<T>(std::forward<U>(default_value));
    }

    // Monadic operations with improved efficiency
    template <typename F>
    constexpr auto map(F&& f) const& -> Expected<std::invoke_result_t<F, const T&>, E>
    {
        using U = std::invoke_result_t<F, const T&>;
        if (has_value()) {
            if constexpr (std::is_void_v<U>) {
                std::invoke(std::forward<F>(f), **this);
                return Expected<U, E>{std::in_place};
            } else {
                return Expected<U, E>{std::invoke(std::forward<F>(f), **this)};
            }
        } else {
            return Expected<U, E>{Unexpected{error()}};
        }
    }

    template <typename F>
    constexpr auto map(F&& f) && -> Expected<std::invoke_result_t<F, T&&>, E>
    {
        using U = std::invoke_result_t<F, T&&>;
        if (has_value()) {
            if constexpr (std::is_void_v<U>) {
                std::invoke(std::forward<F>(f), std::move(**this));
                return Expected<U, E>{std::in_place};
            } else {
                return Expected<U, E>{std::invoke(std::forward<F>(f), std::move(**this))};
            }
        } else {
            return Expected<U, E>{Unexpected{std::move(error())}};
        }
    }

    // and_then (flatMap) for chaining Expected operations
    template <typename F>
    constexpr auto and_then(F&& f) const& -> std::invoke_result_t<F, const T&>
    {
        using U = std::invoke_result_t<F, const T&>;
        static_assert(std::is_same_v<typename U::error_type, E>, 
                     "Error types must match for and_then");
        
        if (has_value()) {
            return std::invoke(std::forward<F>(f), **this);
        } else {
            return U{Unexpected{error()}};
        }
    }

    template <typename F>
    constexpr auto and_then(F&& f) && -> std::invoke_result_t<F, T&&>
    {
        using U = std::invoke_result_t<F, T&&>;
        static_assert(std::is_same_v<typename U::error_type, E>, 
                     "Error types must match for and_then");
        
        if (has_value()) {
            return std::invoke(std::forward<F>(f), std::move(**this));
        } else {
            return U{Unexpected{std::move(error())}};
        }
    }

    // or_else for error handling
    template <typename F>
    constexpr auto or_else(F&& f) const& -> std::invoke_result_t<F, const E&>
    {
        if (has_value()) {
            using NextExpected = std::invoke_result_t<F, const E&>;
            // Ensure that if we have a value, it's convertible to the value_type of NextExpected
            // or NextExpected can be constructed from our T.
            // This typically means NextExpected::value_type must be T or constructible from T.
            // And NextExpected::error_type must be compatible if we were to construct an error case.
            // For simplicity, we assume NextExpected is constructible with our value T.
            // This is a common pattern for or_else where the value path should seamlessly transition.
            static_assert(std::is_constructible_v<NextExpected, const T&>,
                          "The type returned by the function argument 'f' in or_else (when no error) must be constructible from the original Expected's value type.");
            return NextExpected(**this);
        } else {
            return std::invoke(std::forward<F>(f), error());
        }
    }

    template <typename F>
    constexpr auto or_else(F&& f) && -> std::invoke_result_t<F, E&&>
    {
        if (has_value()) {
            using NextExpected = std::invoke_result_t<F, E&&>;
            static_assert(std::is_constructible_v<NextExpected, T&&>,
                          "The type returned by the function argument 'f' in or_else (when no error) must be constructible from the original Expected's value type (rvalue).");
            return NextExpected(std::move(**this));
        } else {
            return std::invoke(std::forward<F>(f), std::move(error()));
        }
    }

    // map_error for transforming errors
    template <typename F>
    constexpr auto map_error(F&& f) const& -> Expected<T, std::invoke_result_t<F, const E&>>
    {
        using G = std::invoke_result_t<F, const E&>;
        if (has_value()) {
            return Expected<T, G>{**this};
        } else {
            return Expected<T, G>{Unexpected{std::invoke(std::forward<F>(f), error())}};
        }
    }

    template <typename F>
    constexpr auto map_error(F&& f) && -> Expected<T, std::invoke_result_t<F, E&&>>
    {
        using G = std::invoke_result_t<F, E&&>;
        if (has_value()) {
            return Expected<T, G>{std::move(**this)};
        } else {
            return Expected<T, G>{Unexpected{std::invoke(std::forward<F>(f), std::move(error()))}};
        }
    }

    // Swap
    void swap(Expected& other) noexcept(std::is_nothrow_move_constructible_v<T> && 
                                       std::is_nothrow_move_constructible_v<E> &&
                                       std::is_nothrow_swappable_v<T> && 
                                       std::is_nothrow_swappable_v<E>)
    {
        using std::swap;
        swap(m_value_or_error, other.m_value_or_error);
    }

    // Equality operators
    template <typename T2, typename E2>
    constexpr bool operator==(const Expected<T2, E2>& other) const
        noexcept(noexcept(std::declval<T>() == std::declval<T2>()) && 
                 noexcept(std::declval<E>() == std::declval<E2>()))
    {
        if (has_value() != other.has_value()) {
            return false;
        }
        return has_value() ? (**this == *other) : (error() == other.error());
    }

    template <typename T2, typename E2>
    constexpr bool operator!=(const Expected<T2, E2>& other) const
        noexcept(noexcept(!(*this == other)))
    {
        return !(*this == other);
    }

    // Compare with values
    template <typename T2>
    constexpr bool operator==(const T2& value) const
        noexcept(noexcept(std::declval<T>() == value))
    {
        return has_value() && (**this == value);
    }

    template <typename T2>
    constexpr bool operator!=(const T2& value) const
        noexcept(noexcept(!(*this == value)))
    {
        return !(*this == value);
    }

    // Compare with Unexpected
    template <typename E2>
    constexpr bool operator==(const Unexpected<E2>& unexpected) const
        noexcept(noexcept(std::declval<E>() == unexpected.error()))
    {
        return !has_value() && (error() == unexpected.error());
    }

    template <typename E2>
    constexpr bool operator!=(const Unexpected<E2>& unexpected) const
        noexcept(noexcept(!(*this == unexpected)))
    {
        return !(*this == unexpected);
    }

private:
    std::variant<T, E> m_value_or_error;
};

// Deduction guides
template <typename T>
Expected(T) -> Expected<T>;

template <typename T, typename E>
Expected(Unexpected<E>) -> Expected<T, E>;

// Non-member functions

/**
 * @brief Creates an `Unexpected` object with a given error.
 */
template <typename E>
constexpr Unexpected<std::decay_t<E>> make_unexpected(E&& error)
    noexcept(std::is_nothrow_constructible_v<std::decay_t<E>, E>)
{
    return Unexpected<std::decay_t<E>>(std::forward<E>(error));
}

/**
 * @brief Executes a function and returns an `Expected` object containing the result or an error.
 */
template <typename Func, typename E = std::string>
constexpr auto make_expected(Func&& func) noexcept -> Expected<std::invoke_result_t<Func>, E>
{
    try {
        if constexpr (std::is_void_v<std::invoke_result_t<Func>>) {
            std::invoke(std::forward<Func>(func));
            return Expected<void, E>{std::in_place};
        } else {
            return Expected<std::invoke_result_t<Func>, E>{std::invoke(std::forward<Func>(func))};
        }
    }
    catch (const std::exception& e) {
        return Expected<std::invoke_result_t<Func>, E>{make_unexpected(E{e.what()})};
    }
    catch (...) {
        return Expected<std::invoke_result_t<Func>, E>{make_unexpected(E{"Unknown exception"})};
    }
}

// Swap function
template <typename T, typename E>
void swap(Expected<T, E>& lhs, Expected<T, E>& rhs)
    noexcept(noexcept(lhs.swap(rhs)))
{
    lhs.swap(rhs);
}

} // namespace aos_utils

#endif
