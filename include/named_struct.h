#ifndef NAMED_STRUCT_HPP
#define NAMED_STRUCT_HPP

#include <iostream>
#include <string>
#include <tuple>
#include <utility>
#include <string_view>
#include <type_traits>
#include <concepts>

// A helper for compile-time strings
template <size_t N>
struct StringLiteral {
    constexpr StringLiteral(const char (&str)[N]) {
        std::copy_n(str, N, value);
    }
    char value[N];
    
    constexpr operator std::string_view() const {
        return std::string_view(value, N - 1); // N-1 to exclude null terminator
    }
};

// Represents a single field in our NamedStruct
template <StringLiteral Name, typename Type>
struct Field {
    using type = Type;
    static constexpr const char* name = Name.value;
    static constexpr std::string_view name_view = Name;
    Type value;
    
    // Default constructor
    Field() = default;
    
    // Constructor from value
    template <typename U>
    requires std::convertible_to<U, Type>
    Field(U&& val) : value(std::forward<U>(val)) {}
};

// Helper to find field index by name
template <StringLiteral Name, typename... Fields>
constexpr size_t find_field_index() {
    size_t index = 0;
    bool found = false;
    ((std::string_view(Fields::name_view) == std::string_view(Name) ? 
      (found = true, true) : (found ? true : (++index, false))), ...);
    return index;
}

// The core NamedStruct template, built on a variadic pack of Fields
template <typename... Fields>
struct NamedStruct : Fields... {
    static_assert(sizeof...(Fields) > 0, "NamedStruct must have at least one field");
    
    // Default constructor
    NamedStruct() = default;
    
    // Constructor to initialize all fields by position
    template <typename... Args>
    requires (sizeof...(Args) == sizeof...(Fields)) && 
             (std::convertible_to<Args, typename Fields::type> && ...)
    NamedStruct(Args&&... args) : Fields{std::forward<Args>(args)}... {}
    
    // Get a field by its index - returns the value directly
    template <size_t I>
    auto& get() {
        static_assert(I < sizeof...(Fields), "Field index out of bounds");
        return std::get<I>(std::tie(static_cast<Fields&>(*this).value...));
    }
    
    template <size_t I>
    const auto& get() const {
        static_assert(I < sizeof...(Fields), "Field index out of bounds");
        return std::get<I>(std::tie(static_cast<const Fields&>(*this).value...));
    }
    
    // Get a field by its name - returns the value directly
    template <StringLiteral Name>
    auto& get() {
        constexpr size_t index = find_field_index<Name, Fields...>();
        static_assert(index < sizeof...(Fields), "Field name not found");
        return get<index>();
    }
    
    template <StringLiteral Name>
    const auto& get() const {
        constexpr size_t index = find_field_index<Name, Fields...>();
        static_assert(index < sizeof...(Fields), "Field name not found");
        return get<index>();
    }
    
    // Get field name by index
    template <size_t I>
    static constexpr const char* field_name() {
        static_assert(I < sizeof...(Fields), "Field index out of bounds");
        return std::get<I>(std::make_tuple(Fields::name...));
    }
    
    // Number of fields
    static constexpr size_t size() {
        return sizeof...(Fields);
    }
    
    // Convert to tuple
    auto as_tuple() & {
        return std::tie(static_cast<Fields&>(*this).value...);
    }
    
    auto as_tuple() const& {
        return std::tie(static_cast<const Fields&>(*this).value...);
    }
    
    auto as_tuple() && {
        return std::make_tuple(std::move(static_cast<Fields&>(*this).value)...);
    }
};

// Structured binding support - these need to be in std namespace
namespace std {
    template <typename... Fields>
    struct tuple_size<NamedStruct<Fields...>> : integral_constant<size_t, sizeof...(Fields)> {};
    
    template <size_t I, typename... Fields>
    struct tuple_element<I, NamedStruct<Fields...>> {
        using type = decay_t<decltype(declval<NamedStruct<Fields...>>().template get<I>())>;
    };
}

// ADL-friendly get function for structured bindings
template <size_t I, typename... Fields>
auto& get(NamedStruct<Fields...>& ns) {
    return ns.template get<I>();
}

template <size_t I, typename... Fields>
const auto& get(const NamedStruct<Fields...>& ns) {
    return ns.template get<I>();
}

template <size_t I, typename... Fields>
auto get(NamedStruct<Fields...>&& ns) {
    return std::move(ns).template get<I>();
}

// A helper to make defining NamedStructs easier
#define NAMED_STRUCT(name, ...) \
    using name = NamedStruct<__VA_ARGS__>;

// A helper for defining fields
#define FIELD(name, type) Field<name, type>

// --- Utility Functions ---

// Pretty printer for any NamedStruct
template <typename... Fields>
std::ostream& operator<<(std::ostream& os, const NamedStruct<Fields...>& s) {
    os << "{ ";
    [&]<size_t... I>(std::index_sequence<I...>) {
        bool first = true;
        ([&] {
            if (!first) os << ", ";
            os << s.template field_name<I>() << ": " << s.template get<I>();
            first = false;
        }(), ...);
    }(std::make_index_sequence<sizeof...(Fields)>{});
    os << " }";
    return os;
}

// JSON serialization helper for basic types
template <typename T>
std::string to_json_value(const T& value) {
    if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
        // Basic string escaping - in production, use a proper JSON library
        std::string escaped = "\"";
        for (char c : value) {
            switch (c) {
                case '"': escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default: escaped += c; break;
            }
        }
        escaped += "\"";
        return escaped;
    } else if constexpr (std::is_same_v<std::decay_t<T>, bool>) {
        return value ? "true" : "false";
    } else if constexpr (std::is_arithmetic_v<std::decay_t<T>>) {
        return std::to_string(value);
    } else {
        // Fallback - try to convert to string
        return "\"" + std::to_string(value) + "\"";
    }
}

// JSON serializer for any NamedStruct
template <typename... Fields>
std::string to_json(const NamedStruct<Fields...>& s) {
    std::string json = "{ ";
    [&]<size_t... I>(std::index_sequence<I...>) {
        bool first = true;
        ([&] {
            if (!first) json += ", ";
            json += "\"" + std::string(s.template field_name<I>()) + "\": ";
            json += to_json_value(s.template get<I>());
            first = false;
        }(), ...);
    }(std::make_index_sequence<sizeof...(Fields)>{});
    json += " }";
    return json;
}

// Equality comparison
template <typename... Fields>
bool operator==(const NamedStruct<Fields...>& lhs, const NamedStruct<Fields...>& rhs) {
    return lhs.as_tuple() == rhs.as_tuple();
}

template <typename... Fields>
bool operator!=(const NamedStruct<Fields...>& lhs, const NamedStruct<Fields...>& rhs) {
    return !(lhs == rhs);
}

// Less-than comparison for ordering
template <typename... Fields>
bool operator<(const NamedStruct<Fields...>& lhs, const NamedStruct<Fields...>& rhs) {
    return lhs.as_tuple() < rhs.as_tuple();
}

#endif // NAMED_STRUCT_HPP

