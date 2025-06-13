#ifndef NAMED_STRUCT_HPP
#define NAMED_STRUCT_HPP

#include <iostream>
#include <string>
#include <tuple>
#include <utility>
#include <functional> // Added for std::hash
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

// Field mutability options
enum class FieldMutability {
    Mutable,
    Immutable
};

// Represents a single field in our NamedStruct
template <StringLiteral Name, typename Type, FieldMutability Mut = FieldMutability::Mutable>
struct Field {
    using type = Type;
    static constexpr const char* name = Name.value;
    static constexpr std::string_view name_view = Name;
    static constexpr FieldMutability mutability = Mut;
    
    // Use const for immutable fields
    using storage_type = std::conditional_t<Mut == FieldMutability::Immutable, const Type, Type>;
    storage_type value;

    static constexpr Type get_default_value() {
        // For now, we rely on Type being default constructible.
        // More advanced would be to allow specifying a default in FIELD macro.
        return Type{};
    }
    
    // Default constructor
    constexpr Field() : value(get_default_value()) {}
    
    // Constructor from value
    template <typename U>
    requires std::convertible_to<U, Type>
    constexpr Field(U&& val) : value(std::forward<U>(val)) {}
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

// NOTE ON STRUCTURED BINDINGS (especially for g++ 13.1.0 and similar versions):
// Direct structured binding (e.g., auto [x,y] = named_struct_instance;) has shown
// to be unreliable with this NamedStruct implementation on certain compilers
// (like g++ 13.1.0), potentially due to compiler bugs when handling
// template argument deduction for variadic packs, type aliases, and NTTPs within
// std::tuple_size/std::tuple_element specializations, or during the structured
// binding process itself.
//
// RECOMMENDED ALTERNATIVE for decomposition:
// Use the .as_tuple() method and, if needed, apply structured bindings to the
// resulting std::tuple. However, even this indirect approach has shown issues
// on affected compilers in some contexts.
//
// SAFEST ALTERNATIVE for field access:
// Use direct member access:
//   my_instance.get<"field_name">()
//   my_instance.get<index>()
//
// Or, for converting to a tuple and accessing its elements manually:
//   auto t = my_instance.as_tuple();
//   auto x = std::get<0>(t);
//   auto y = std::get<1>(t);
//
// Custom std::tuple_size, std::tuple_element, and ADL get() functions for NamedStruct
// have been removed to avoid these compiler issues.

// The core NamedStruct template, built on a variadic pack of Fields
template <typename... Fields>
struct NamedStruct : Fields... {
    static_assert(sizeof...(Fields) > 0, "NamedStruct must have at least one field");
    
    // Default constructor
    constexpr NamedStruct() : Fields()... {}
    
    // Constructor to initialize all fields by position
    template <typename... Args>
    requires (sizeof...(Args) == sizeof...(Fields)) && 
             (std::convertible_to<Args, typename Fields::type> && ...)
    constexpr NamedStruct(Args&&... args) : Fields{std::forward<Args>(args)}... {}
    
    // Get a field by its index - returns the value directly
    template <size_t I>
    constexpr auto& get() {
        static_assert(I < sizeof...(Fields), "Field index out of bounds");
        return std::get<I>(std::tie(static_cast<Fields&>(*this).value...));
    }
    
    template <size_t I>
    constexpr const auto& get() const {
        static_assert(I < sizeof...(Fields), "Field index out of bounds");
        return std::get<I>(std::tie(static_cast<const Fields&>(*this).value...));
    }
    
    // Get a field by its name - returns the value directly
    template <StringLiteral Name>
    constexpr auto& get() {
        constexpr size_t index = find_field_index<Name, Fields...>();
        static_assert(index < sizeof...(Fields), "Field name not found");
        return get<index>();
    }
    
    template <StringLiteral Name>
    constexpr const auto& get() const {
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
    
    // Setters for mutable fields only
    template <size_t I, typename U>
    void set(U&& value) {
        static_assert(I < sizeof...(Fields), "Field index out of bounds");
        using FieldType = std::tuple_element_t<I, std::tuple<Fields...>>;
        static_assert(FieldType::mutability == FieldMutability::Mutable, "Cannot modify immutable field");
        get<I>() = std::forward<U>(value);
    }
    
    template <StringLiteral Name, typename U>
    void set(U&& value) {
        constexpr size_t index = find_field_index<Name, Fields...>();
        static_assert(index < sizeof...(Fields), "Field name not found");
        set<index>(std::forward<U>(value));
    }
    
    // Check if field is mutable
    template <size_t I>
    static constexpr bool is_mutable() {
        static_assert(I < sizeof...(Fields), "Field index out of bounds");
        using FieldType = std::tuple_element_t<I, std::tuple<Fields...>>;
        return FieldType::mutability == FieldMutability::Mutable;
    }
    
    template <StringLiteral Name>
    static constexpr bool is_mutable() {
        constexpr size_t index = find_field_index<Name, Fields...>();
        static_assert(index < sizeof...(Fields), "Field name not found");
        return is_mutable<index>();
    }
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

// A helper to make defining NamedStructs easier
#define NAMED_STRUCT(name, ...) \
    using name = NamedStruct<__VA_ARGS__>;

// A helper for defining fields
#define FIELD(name, type) Field<name, type, FieldMutability::Mutable>
#define IMMUTABLE_FIELD(name, type) Field<name, type, FieldMutability::Immutable>
#define MUTABLE_FIELD(name, type) Field<name, type, FieldMutability::Mutable>

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

// Greater than
template <typename... Fields>
bool operator>(const NamedStruct<Fields...>& lhs, const NamedStruct<Fields...>& rhs) {
    return rhs < lhs;
}

// Less than or equal to
template <typename... Fields>
bool operator<=(const NamedStruct<Fields...>& lhs, const NamedStruct<Fields...>& rhs) {
    return !(rhs < lhs); // Using !(rhs < lhs) is equivalent to lhs < rhs || lhs == rhs
}

// Greater than or equal to
template <typename... Fields>
bool operator>=(const NamedStruct<Fields...>& lhs, const NamedStruct<Fields...>& rhs) {
    return !(lhs < rhs); // Using !(lhs < rhs) is equivalent to lhs > rhs || lhs == rhs
}

// --- Hashing Support ---
namespace std {
    template <typename... Fields>
    struct hash<NamedStruct<Fields...>> {
        size_t operator()(const NamedStruct<Fields...>& s) const {
            size_t seed = 0;
            // Helper lambda to combine hashes
            auto hash_combiner = [&](const auto& value) {
                using ValueType = std::decay_t<decltype(value)>;
                seed ^= std::hash<ValueType>{}(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            };

            // Iterate over fields and apply combiner
            [&]<size_t... I>(std::index_sequence<I...>) {
                (hash_combiner(s.template get<I>()), ...);
            }(std::make_index_sequence<sizeof...(Fields)>{});

            return seed;
        }
    };
} // namespace std

#endif // NAMED_STRUCT_HPP

