#pragma once

#include <tuple>
#include <utility>
#include <functional>
#include <type_traits>

namespace functools {

// Forward declaration
template<typename F, typename... BoundArgs>
class partial_impl;

namespace detail {
    // Helper to check if a type is a partial object
    template<typename T>
    struct is_partial : std::false_type {};
    
    template<typename F, typename... Args>
    struct is_partial<partial_impl<F, Args...>> : std::true_type {};
    
    // Helper for perfect forwarding in tuple
    template<typename T>
    using decay_if_not_ref = std::conditional_t<
        std::is_reference_v<T>, T, std::decay_t<T>
    >;
    
    // Index sequence helpers for C++14 compatibility
    template<std::size_t... Is>
    using index_sequence = std::index_sequence<Is...>;
    
    template<std::size_t N>
    using make_index_sequence = std::make_index_sequence<N>;
}

// Main partial implementation class
// Forward declaration for the partial free function
template<typename F, typename... Args>
class partial_impl;

namespace detail {
    // is_partial defined earlier

    // Helper to apply callable and a tuple of args to partial_impl constructor
    template<typename Callable, typename TupleArgs, std::size_t... Is>
    constexpr auto make_partial_with_args_tuple(Callable&& c, TupleArgs&& t, std::index_sequence<Is...>) {
        // Ensure this doesn't try to re-wrap a partial_impl in another partial_impl's func_
        // The constructor of partial_impl should be SFINAE'd to prevent F from being a partial_impl type.
        return functools::partial_impl<std::decay_t<Callable>, std::decay_t<std::tuple_element_t<Is, std::decay_t<TupleArgs>>>...>(
            std::forward<Callable>(c),
            std::get<Is>(std::forward<TupleArgs>(t))...
        );
    }
} // namespace detail


template<typename F, typename... BoundArgs>
class partial_impl {
private:
    F func_; // This F should never be a partial_impl type due to SFINAE on constructor
    std::tuple<detail::decay_if_not_ref<BoundArgs>...> bound_args_;
    
    // Helper to invoke with bound + remaining args
    template<std::size_t... Is, typename... RemainingArgs>
    constexpr decltype(auto) invoke_impl(
        detail::index_sequence<Is...>,
        RemainingArgs&&... remaining_args
    ) const {
        if constexpr (std::is_member_function_pointer_v<std::decay_t<F>>) {
            // Handle member function pointers
            return std::invoke(func_, std::get<Is>(bound_args_)..., 
                             std::forward<RemainingArgs>(remaining_args)...);
        } else {
            // Handle regular callables (including other partial objects)
            return std::invoke(func_, std::get<Is>(bound_args_)..., 
                             std::forward<RemainingArgs>(remaining_args)...);
        }
    }

public:
    // SFINAE'd constructor to ensure F (the callable type) is not another partial_impl.
    // The functools::partial free function is responsible for unwrapping.
    template<
        typename FuncCvRef,
        typename... CallArgs,
        // SFINAE condition moved to template parameter list
        typename = std::enable_if_t<!detail::is_partial<std::decay_t<FuncCvRef>>::value>
    >
    constexpr explicit partial_impl(FuncCvRef&& f, CallArgs&&... call_args)
        : func_(std::forward<FuncCvRef>(f))
        , bound_args_(std::forward<CallArgs>(call_args)...) {}
    
    // Copy constructor
    partial_impl(const partial_impl&) = default;
    
    // Move constructor  
    partial_impl(partial_impl&&) = default;
    
    // Call operator
    template<typename... RemainingArgs>
    constexpr decltype(auto) operator()(RemainingArgs&&... remaining_args) const {
        return invoke_impl(
            detail::make_index_sequence<sizeof...(BoundArgs)>{},
            std::forward<RemainingArgs>(remaining_args)...
        );
    }
    
    // Allow conversion to std::function when types are compatible
    template<typename R, typename... Args>
    operator std::function<R(Args...)>() const {
        return [*this](Args... args) -> R {
            return (*this)(std::forward<Args>(args)...);
        };
    }

    // Accessors for callable and bound arguments (needed for unwrapping)
    // Ref-qualified getters to handle different value categories of the partial_impl object.
    constexpr const F& get_callable() const & noexcept { return func_; }
    constexpr F& get_callable() & noexcept { return func_; } // Non-const lvalue access
    constexpr F&& get_callable() && noexcept { return std::move(func_); } // Rvalue access (move from)

    constexpr const std::tuple<detail::decay_if_not_ref<BoundArgs>...>& get_bound_args_tuple() const & noexcept {
        return bound_args_;
    }
    // No non-const & overload for bound_args_tuple to simplify; const & is usually sufficient for reading.
    // The && overload allows moving the tuple if the partial_impl object is an rvalue.
    constexpr std::tuple<detail::decay_if_not_ref<BoundArgs>...>&& get_bound_args_tuple() && noexcept {
        return std::move(bound_args_);
    }
};

// Deduction guide for the partial_impl class
template<typename F, typename... Args>
partial_impl(F&&, Args&&...) -> partial_impl<std::decay_t<F>, std::decay_t<Args>...>;

// Main partial function (definition moved after detail helpers)
template<typename F, typename... Args>
constexpr auto partial(F&& func, Args&&... args);


namespace detail {
    // Helper to apply a tuple to a constructor of partial_impl
    // (Moved to detail namespace and refined)
    template<typename FUnwrapped, typename Tuple, std::size_t... Is>
    constexpr auto make_partial_from_tuple_impl(FUnwrapped&& func_unwrapped, Tuple&& tuple, std::index_sequence<Is...>) {
        // Correctly deduce types for partial_impl template arguments from the tuple elements
        return functools::partial_impl<
            std::decay_t<FUnwrapped>,
            std::decay_t<std::tuple_element_t<Is, std::decay_t<Tuple>>>...
        >(
            std::forward<FUnwrapped>(func_unwrapped),
            std::get<Is>(std::forward<Tuple>(tuple))...
        );
    }
} // namespace detail


// Definition of partial function
template<typename F, typename... Args>
constexpr auto partial(F&& func, Args&&... args) {
    using DecayedF = std::decay_t<F>;
    if constexpr (detail::is_partial<DecayedF>::value) {
        // func is a partial_impl. Unwrap and re-bind.
        // The make_partial_with_args_tuple helper will call the SFINAE'd partial_impl constructor.
        // It's crucial that get_callable() and get_bound_args_tuple() correctly propagate
        // the value category of 'func' (lvalue or rvalue).
        return detail::make_partial_with_args_tuple(
            std::forward<F>(func).get_callable(), // Gets the inner callable
            std::tuple_cat(                       // Concatenates args
                std::forward<F>(func).get_bound_args_tuple(), // Inner args
                std::make_tuple(std::forward<Args>(args)...)    // New args
            ),
            // Create index sequence for the size of the concatenated tuple
            std::make_index_sequence<
                std::tuple_size_v<std::decay_t<decltype(std::forward<F>(func).get_bound_args_tuple())>> +
                sizeof...(Args)
            >{}
        );
    } else {
        // Base case: func is not a partial_impl.
        // Directly construct partial_impl (its constructor is SFINAE'd to ensure F is not partial).
        return functools::partial_impl<DecayedF, std::decay_t<Args>...>(
            std::forward<F>(func), std::forward<Args>(args)...);
    }
}

// Convenience alias for partial objects
template<typename F, typename... BoundArgs>
using Partial = partial_impl<F, BoundArgs...>;

} // namespace functools
