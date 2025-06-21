#pragma once

#include <array>
#include <string_view>
#include <type_traits>
#include <algorithm>
#include <optional>
#include <stdexcept>

// Feature detection
#if __cplusplus >= 202002L
    #define ENUM_REFLECT_CPP20
#elif __cplusplus >= 201703L
    #define ENUM_REFLECT_CPP17
#else
    #error "C++17 or later required"
#endif

namespace enum_reflect {

namespace detail {
    // Helper to check if a value is a valid enum value
    template<typename E>
    constexpr bool is_valid_enum_value(std::underlying_type_t<E> value) {
        return value >= 0 && value < 256; // Reasonable range for most enums
    }

    // Extract enum name from function signature
    template<typename E, E V>
    constexpr std::string_view get_enum_name_impl() {
#if defined(__GNUC__) || defined(__clang__)
        constexpr std::string_view prefix = "E V = ";
        constexpr std::string_view suffix = "]";
        constexpr std::string_view function = __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
        constexpr std::string_view prefix = "get_enum_name_impl<";
        constexpr std::string_view suffix = ">(void)";
        constexpr std::string_view function = __FUNCSIG__;
#else
        return "UNKNOWN";
#endif
        
        constexpr auto start = function.find(prefix);
        if constexpr (start == std::string_view::npos) {
            return "UNKNOWN";
        }
        
        constexpr auto name_start = start + prefix.length();
        constexpr auto end = function.find(suffix, name_start);
        if constexpr (end == std::string_view::npos) {
            return "UNKNOWN";
        }
        
        constexpr auto name = function.substr(name_start, end - name_start);
        
        // Extract just the enum value name (after last ::)
        constexpr auto last_colon = name.find_last_of(':');
        if constexpr (last_colon != std::string_view::npos) {
            return name.substr(last_colon + 1);
        }
        
        return name;
    }

    // Check if enum value has a valid name (not a number)
    template<typename E, E V>
    constexpr bool has_valid_name() {
        constexpr auto name = get_enum_name_impl<E, V>();
        return !name.empty() && 
               name != "UNKNOWN" && 
               (name[0] < '0' || name[0] > '9'); // Not a numeric literal
    }

    // Generate sequence of enum values
    template<typename E, std::underlying_type_t<E>... Is>
    constexpr auto make_enum_array(std::integer_sequence<std::underlying_type_t<E>, Is...>) {
        return std::array<E, sizeof...(Is)>{static_cast<E>(Is)...};
    }

    // Filter valid enum values
    template<typename E, std::size_t N>
    constexpr auto filter_valid_enums(const std::array<E, N>& candidates) {
        std::array<E, N> result{};
        std::size_t count = 0;
        
        for (const auto& candidate : candidates) {
            if constexpr (N > 0) {
                // This is a compile-time check that will be optimized
                constexpr auto first_val = static_cast<std::underlying_type_t<E>>(candidates[0]);
                if (has_valid_name<E, static_cast<E>(first_val)>()) {
                    result[count++] = candidate;
                }
            }
        }
        
        // Return only the valid portion
        std::array<E, N> final_result{};
        for (std::size_t i = 0; i < count && i < N; ++i) {
            final_result[i] = result[i];
        }
        return final_result;
    }

#ifdef ENUM_REFLECT_CPP20
    // C++20 version with concepts
    template<typename T>
    concept EnumType = std::is_enum_v<T>;
    
    template<EnumType E, E V>
    constexpr auto enum_name = get_enum_name_impl<E, V>();
#endif

} // namespace detail

// Main enum reflection class
template<typename E>
class enum_info {
    static_assert(std::is_enum_v<E>, "Template parameter must be an enum type");
    
public:
    using enum_type = E;
    using underlying_type = std::underlying_type_t<E>;
    
    // Get enum name as string
    template<E V>
    static constexpr std::string_view name() {
        return detail::get_enum_name_impl<E, V>();
    }
    
    // Get enum name from value (runtime)
    static constexpr std::string_view name(E value) {
        return name_impl(value, std::make_integer_sequence<underlying_type, 256>{});
    }
    
    // Get enum value from name
    static constexpr std::optional<E> from_name(std::string_view name) {
        return from_name_impl(name, std::make_integer_sequence<underlying_type, 256>{});
    }
    
    // Check if value is valid enum
    static constexpr bool is_valid(E value) {
        return name(value) != "UNKNOWN";
    }
    
    // Get all enum values
    static constexpr auto values() {
        return get_values_impl(std::make_integer_sequence<underlying_type, 256>{});
    }
    
    // Get all enum names
    static constexpr auto names() {
        return get_names_impl(std::make_integer_sequence<underlying_type, 256>{});
    }
    
    // Get count of enum values
    static constexpr std::size_t size() {
        return count_valid_enums(std::make_integer_sequence<underlying_type, 256>{});
    }
    
    // Convert to string (same as name)
    static constexpr std::string_view to_string(E value) {
        return name(value);
    }
    
    // Convert from string (same as from_name)
    static constexpr std::optional<E> from_string(std::string_view str) {
        return from_name(str);
    }

private:
    template<underlying_type... Is>
    static constexpr std::string_view name_impl(E value, std::integer_sequence<underlying_type, Is...>) {
        std::string_view result = "UNKNOWN";
        ((static_cast<underlying_type>(value) == Is && detail::has_valid_name<E, static_cast<E>(Is)>() ? 
          (result = detail::get_enum_name_impl<E, static_cast<E>(Is)>(), true) : false) || ...);
        return result;
    }
    
    template<underlying_type... Is>
    static constexpr std::optional<E> from_name_impl(std::string_view name, std::integer_sequence<underlying_type, Is...>) {
        std::optional<E> result;
        ((detail::has_valid_name<E, static_cast<E>(Is)>() && 
          detail::get_enum_name_impl<E, static_cast<E>(Is)>() == name) ? 
         (result = static_cast<E>(Is), true) : false) || ...);
        return result;
    }
    
    template<underlying_type... Is>
    static constexpr auto get_values_impl(std::integer_sequence<underlying_type, Is...>) {
        constexpr std::size_t count = ((detail::has_valid_name<E, static_cast<E>(Is)>() ? 1 : 0) + ...);
        std::array<E, count> result{};
        std::size_t index = 0;
        ((detail::has_valid_name<E, static_cast<E>(Is)>() ? 
          (result[index++] = static_cast<E>(Is), true) : false) || ...);
        return result;
    }
    
    template<underlying_type... Is>
    static constexpr auto get_names_impl(std::integer_sequence<underlying_type, Is...>) {
        constexpr std::size_t count = ((detail::has_valid_name<E, static_cast<E>(Is)>() ? 1 : 0) + ...);
        std::array<std::string_view, count> result{};
        std::size_t index = 0;
        ((detail::has_valid_name<E, static_cast<E>(Is)>() ? 
          (result[index++] = detail::get_enum_name_impl<E, static_cast<E>(Is)>(), true) : false) || ...);
        return result;
    }
    
    template<underlying_type... Is>
    static constexpr std::size_t count_valid_enums(std::integer_sequence<underlying_type, Is...>) {
        return ((detail::has_valid_name<E, static_cast<E>(Is)>() ? 1 : 0) + ...);
    }
};

// Convenience functions
template<typename E>
constexpr std::string_view enum_name(E value) {
    return enum_info<E>::name(value);
}

template<typename E>
constexpr std::optional<E> enum_from_name(std::string_view name) {
    return enum_info<E>::from_name(name);
}

template<typename E>
constexpr auto enum_values() {
    return enum_info<E>::values();
}

template<typename E>
constexpr auto enum_names() {
    return enum_info<E>::names();
}

template<typename E>
constexpr std::size_t enum_size() {
    return enum_info<E>::size();
}

template<typename E>
constexpr bool is_valid_enum(E value) {
    return enum_info<E>::is_valid(value);
}

// Iterator support for range-based for loops
template<typename E>
class enum_iterator {
    using values_array = decltype(enum_info<E>::values());
    const values_array& values_;
    std::size_t index_;
    
public:
    constexpr enum_iterator(const values_array& values, std::size_t index) 
        : values_(values), index_(index) {}
    
    constexpr E operator*() const { return values_[index_]; }
    constexpr enum_iterator& operator++() { ++index_; return *this; }
    constexpr enum_iterator operator++(int) { auto tmp = *this; ++index_; return tmp; }
    constexpr bool operator==(const enum_iterator& other) const { return index_ == other.index_; }
    constexpr bool operator!=(const enum_iterator& other) const { return index_ != other.index_; }
};

template<typename E>
constexpr auto enum_range() {
    struct enum_range_impl {
        static constexpr auto values = enum_info<E>::values();
        
        constexpr auto begin() const { return enum_iterator<E>(values, 0); }
        constexpr auto end() const { return enum_iterator<E>(values, values.size()); }
    };
    return enum_range_impl{};
}

// Stream output operator
#ifdef ENUM_REFLECT_CPP20
template<detail::EnumType E>
#else
template<typename E, typename = std::enable_if_t<std::is_enum_v<E>>>
#endif
std::ostream& operator<<(std::ostream& os, E value) {
    return os << enum_name(value);
}

} // namespace enum_reflect

// Convenience macros
#define ENUM_REFLECT_REGISTER(EnumType) \
    namespace enum_reflect { \
        template<> \
        struct enum_info<EnumType> : public enum_info<EnumType> {}; \
    }

// Usage example and verification
#ifdef ENUM_REFLECT_EXAMPLE
#include <iostream>

enum class Status {
    PENDING = 0,
    RUNNING = 1,
    COMPLETE = 2,
    ERROR = 10
};

enum class LogLevel : int {
    DEBUG,
    INFO, 
    WARN,
    ERROR
};

void example_usage() {
    using namespace enum_reflect;
    
    // Basic usage
    std::cout << "Status::PENDING name: " << enum_name(Status::PENDING) << std::endl;
    std::cout << "LogLevel::INFO name: " << enum_name(LogLevel::INFO) << std::endl;
    
    // From string
    auto status = enum_from_name<Status>("RUNNING");
    if (status) {
        std::cout << "Found status: " << enum_name(*status) << std::endl;
    }
    
    // Iteration
    std::cout << "All Status values:" << std::endl;
    for (auto s : enum_range<Status>()) {
        std::cout << "  " << enum_name(s) << " = " << static_cast<int>(s) << std::endl;
    }
    
    // All values at once
    constexpr auto all_levels = enum_values<LogLevel>();
    std::cout << "LogLevel count: " << all_levels.size() << std::endl;
    
    // Validation
    std::cout << "Is Status::ERROR valid? " << is_valid_enum(Status::ERROR) << std::endl;
    
    // Stream output
    std::cout << "Direct output: " << Status::COMPLETE << std::endl;
}
#endif
