#ifndef ONE_OF_H
#define ONE_OF_H

#include <variant>   // For std::variant_npos, std::bad_variant_access
#include <type_traits>
#include <typeinfo>  // For typeid operator
#include <typeindex> // For std::type_index
#include <stdexcept> // For std::logic_error, std::runtime_error (though bad_variant_access is from <variant>)
#include <utility>   // For std::in_place_type_t, std::forward, std::move
#include <cstddef>   // For std::byte, std::size_t
#include <new>       // For placement new
#include <memory>    // For std::destroy_at
#include <algorithm> // For std::max (for initializer list version)
#include <numeric>   // For potential future use or alternative for max over pack (though not used here now)


// Helper to get the index of a type in a parameter pack
template<typename T, typename... Ts>
struct type_index_in_pack;

template<typename T, typename First, typename... Rest>
struct type_index_in_pack<T, First, Rest...> {
    static constexpr std::size_t value = std::is_same_v<T, First> ? 0 : (type_index_in_pack<T, Rest...>::value + 1);
};

template<typename T>
struct type_index_in_pack<T> {
    // This value indicates T was not found after exhausting the pack.
    // It's large enough that adding 1 repeatedly won't wrap around to a small valid index for typical pack sizes.
    static constexpr std::size_t value = static_cast<std::size_t>(-1) / 2;
};

template<typename T, typename... Ts>
inline constexpr std::size_t type_index_in_pack_v = type_index_in_pack<T, Ts...>::value;

// Helper to check if a type is in a parameter pack
template<typename T, typename... Ts>
struct is_one_of_types;

template<typename T, typename First, typename... Rest>
struct is_one_of_types<T, First, Rest...> : std::conditional_t<std::is_same_v<T, First>, std::true_type, is_one_of_types<T, Rest...>> {};

template<typename T>
struct is_one_of_types<T> : std::false_type {};

template<typename T, typename... Ts>
inline constexpr bool is_one_of_types_v = is_one_of_types<T, Ts...>::value;


template<typename... Ts>
class OneOf {
public:
    // Ensure there's at least one type
    static_assert(sizeof...(Ts) > 0, "OneOf must be instantiated with at least one type.");

    // Compile-time check for duplicate types
private: // Keep helpers private or in a detail namespace
    template<typename T, typename... Haystack>
    struct count_in_pack_helper;

    template<typename T, typename FirstInHaystack, typename... RestOfHaystack>
    struct count_in_pack_helper<T, FirstInHaystack, RestOfHaystack...> : std::integral_constant<std::size_t, (std::is_same_v<T, FirstInHaystack> ? 1 : 0) + count_in_pack_helper<T, RestOfHaystack...>::value> {};

    template<typename T>
    struct count_in_pack_helper<T> : std::integral_constant<std::size_t, 0> {};

    template<typename T, typename... Haystack>
    static constexpr std::size_t count_in_pack_v_helper = count_in_pack_helper<T, Haystack...>::value;

public:
    static_assert(((count_in_pack_v_helper<Ts, Ts...> == 1) && ...), "OneOf cannot be instantiated with duplicate types.");


    OneOf() noexcept
        : active_index_(std::variant_npos) {}

    template<typename T,
             typename DecayedT = std::decay_t<T>, // Deduce DecayedT here
             typename = std::enable_if_t<is_one_of_types_v<DecayedT, Ts...> && !std::is_same_v<std::decay_t<OneOf>, DecayedT>>> // Prevent OneOf(OneOf)
    OneOf(T&& val) : active_index_(std::variant_npos) {
        constexpr std::size_t index = type_index_in_pack_v<DecayedT, Ts...>;
        // This static_assert is technically redundant due to SFINAE but good for clarity.
        static_assert(index < sizeof...(Ts), "Type T not in Ts...");

        new(&storage_) DecayedT(std::forward<T>(val));
        active_index_ = index;
    }

    OneOf(const OneOf& other) : active_index_(std::variant_npos) {
        if (other.has_value()) {
            copy_assign_from(other);
        }
    }

    OneOf(OneOf&& other) noexcept : active_index_(std::variant_npos) {
        if (other.has_value()) {
            move_assign_from(std::move(other));
        }
    }

    OneOf& operator=(const OneOf& other) {
        if (this == &other) {
            return *this;
        }
        reset();
        if (other.has_value()) {
            copy_assign_from(other);
        }
        return *this;
    }

    OneOf& operator=(OneOf&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        reset();
        if (other.has_value()) {
            move_assign_from(std::move(other));
        }
        return *this;
    }

    ~OneOf() {
        reset();
    }

    template<typename T, typename... Args,
             typename DecayedT = std::decay_t<T>, // Deduce DecayedT here
             typename = std::enable_if_t<is_one_of_types_v<DecayedT, Ts...>>>
    DecayedT& emplace(Args&&... args) {
        static_assert(std::is_constructible_v<DecayedT, Args&&...>, "Type T cannot be constructed with the given arguments.");
        constexpr std::size_t index = type_index_in_pack_v<DecayedT, Ts...>;

        reset();

        new(&storage_) DecayedT(std::forward<Args>(args)...);
        active_index_ = index;
        return *reinterpret_cast<DecayedT*>(&storage_);
    }

    template<typename T,
             typename DecayedT = std::decay_t<T>, // Deduce DecayedT here
             typename = std::enable_if_t<is_one_of_types_v<DecayedT, Ts...>>>
    void set(T&& val) {
        constexpr std::size_t index = type_index_in_pack_v<DecayedT, Ts...>;
        // The static_assert for index < sizeof...(Ts) is implicitly covered by
        // is_one_of_types_v in the enable_if, and type_index_in_pack_v returning a valid index.

        reset(); // Always destroy the current contents first.
        new(&storage_) DecayedT(std::forward<T>(val)); // Construct the new value.
        active_index_ = index;
    }

    template<typename T>
    bool has() const noexcept {
        static_assert(is_one_of_types_v<T, Ts...>, "Type T not in Ts...");
        if (!has_value()) {
            return false;
        }
        constexpr std::size_t query_index = type_index_in_pack_v<T, Ts...>;
        return active_index_ == query_index;
    }

    template<typename T>
    T* get_if() noexcept {
        static_assert(is_one_of_types_v<T, Ts...>, "Type T not in Ts...");
        if (!has<T>()) {
            return nullptr;
        }
        return reinterpret_cast<T*>(&storage_);
    }

    template<typename T>
    const T* get_if() const noexcept {
        static_assert(is_one_of_types_v<T, Ts...>, "Type T not in Ts...");
        if (!has<T>()) {
            return nullptr;
        }
        return reinterpret_cast<const T*>(&storage_);
    }

    std::size_t index() const noexcept {
        return active_index_;
    }

    std::type_index type() const {
        if (!has_value()) {
            throw std::bad_variant_access();
        }
        std::type_index ti = std::type_index(typeid(void));
        visit_storage_void(active_index_, [&](const auto* ptr) {
            ti = std::type_index(typeid(decltype(*ptr)));
        });
        return ti;
    }

    template<typename Visitor>
    decltype(auto) visit(Visitor&& visitor) {
        if (!has_value()) {
            throw std::bad_variant_access();
        }
        return call_visitor_recursively<Visitor, Ts...>(active_index_, 0, &storage_, std::forward<Visitor>(visitor));
    }

    template<typename Visitor>
    decltype(auto) visit(Visitor&& visitor) const {
        if (!has_value()) {
            throw std::bad_variant_access();
        }
        return call_visitor_recursively_const<Visitor, Ts...>(active_index_, 0, &storage_, std::forward<Visitor>(visitor));
    }

    bool has_value() const noexcept {
        return active_index_ != std::variant_npos;
    }

    void reset() noexcept {
        if (has_value()) {
            visit_storage_void(active_index_, [](auto* ptr) {
                std::destroy_at(ptr);
            });
            active_index_ = std::variant_npos;
        }
    }

private:
    static constexpr std::size_t get_max_size_impl() {
        if constexpr (sizeof...(Ts) == 0) return 0;
        else {
            std::size_t max_val = 0;
            // This is a C++17 fold expression for max
            ((max_val = (sizeof(Ts) > max_val ? sizeof(Ts) : max_val)), ...);
            return max_val;
        }
    }

    static constexpr std::size_t get_max_align_impl() {
        if constexpr (sizeof...(Ts) == 0) return 1;
        else {
            std::size_t max_val = 1;
            ((max_val = (alignof(Ts) > max_val ? alignof(Ts) : max_val)), ...);
            return max_val;
        }
    }

    alignas(get_max_align_impl()) std::byte storage_[get_max_size_impl()];
    std::size_t active_index_;

    template<std::size_t... Is, typename Func>
    void visit_storage_void_impl(std::size_t current_idx, std::index_sequence<Is...>, Func&& func) {
        (... || (current_idx == Is ? ((void)func(reinterpret_cast<std::tuple_element_t<Is, std::tuple<Ts...>>*>(&storage_)), true) : false));
    }

    template<std::size_t... Is, typename Func>
    void visit_storage_void_impl(std::size_t current_idx, std::index_sequence<Is...>, Func&& func) const {
        (... || (current_idx == Is ? ((void)func(reinterpret_cast<const std::tuple_element_t<Is, std::tuple<Ts...>>*>(&storage_)), true) : false));
    }

    template<typename Func>
    void visit_storage_void(std::size_t current_idx, Func&& func) {
        visit_storage_void_impl(current_idx, std::index_sequence_for<Ts...>{}, std::forward<Func>(func));
    }

    template<typename Func>
    void visit_storage_void(std::size_t current_idx, Func&& func) const {
        visit_storage_void_impl(current_idx, std::index_sequence_for<Ts...>{}, std::forward<Func>(func));
    }

    template<typename Visitor, typename CurrentType, typename... RemainingTypes>
    static decltype(auto) call_visitor_recursively(std::size_t target_idx, std::size_t current_idx, void* data_ptr, Visitor&& vis) {
        if (target_idx == current_idx) {
            return std::forward<Visitor>(vis)(*static_cast<CurrentType*>(data_ptr));
        }
        if constexpr (sizeof...(RemainingTypes) > 0) {
            return call_visitor_recursively<Visitor, RemainingTypes...>(target_idx, current_idx + 1, data_ptr, std::forward<Visitor>(vis));
        } else {
            throw std::logic_error("Internal error: Unreachable code in OneOf::visit dispatch (target_idx out of bounds or logic error)");
        }
    }

    template<typename Visitor, typename CurrentType, typename... RemainingTypes>
    static decltype(auto) call_visitor_recursively_const(std::size_t target_idx, std::size_t current_idx, const void* data_ptr, Visitor&& vis) {
        if (target_idx == current_idx) {
            return std::forward<Visitor>(vis)(*static_cast<const CurrentType*>(data_ptr));
        }
        if constexpr (sizeof...(RemainingTypes) > 0) {
            return call_visitor_recursively_const<Visitor, RemainingTypes...>(target_idx, current_idx + 1, data_ptr, std::forward<Visitor>(vis));
        } else {
             throw std::logic_error("Internal error: Unreachable code in OneOf::visit dispatch (const) (target_idx out of bounds or logic error)");
        }
    }

    void copy_assign_from(const OneOf& other) {
        // Use other.visit_storage_void for dispatching the copy construction
        other.visit_storage_void(other.active_index_, [this](const auto* source_ptr) {
            using HeldType = std::decay_t<decltype(*source_ptr)>;
            new(&storage_) HeldType(*source_ptr);
            active_index_ = type_index_in_pack_v<HeldType, Ts...>;
        });
    }

    void move_assign_from(OneOf&& other) {
        other.visit_storage_void(other.active_index_, [this](auto* source_ptr) {
            using HeldType = std::decay_t<decltype(*source_ptr)>;
            new(&storage_) HeldType(std::move(*source_ptr));
            active_index_ = type_index_in_pack_v<HeldType, Ts...>;
        });
        other.active_index_ = std::variant_npos;
    }
};

#endif // ONE_OF_H
