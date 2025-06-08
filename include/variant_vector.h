#include <vector>
#include <variant>
#include <type_traits>
#include <array>
#include <tuple>
#include <algorithm>
#include <cassert>
#include <memory>
#include <unordered_map>
#include <typeindex>
#include <stdexcept>
#include <unordered_map>
#include <typeindex>

// ============================================================================
// STATIC VARIANT VECTOR (Compile-time known maximum size)
// ============================================================================

template<typename... Types>
class static_variant_vector {
private:
    static constexpr size_t num_types = sizeof...(Types);
    
    // Storage for each type separately (Structure of Arrays)
    std::tuple<std::vector<Types>...> type_vectors;
    
    // Index mapping: global_index -> (type_index, local_index)
    std::vector<std::pair<uint8_t, size_t>> index_map;
    
    template<size_t I>
    using type_at = std::tuple_element_t<I, std::tuple<Types...>>;
    
public:
    using variant_type = std::variant<Types...>;
    
    static_variant_vector() = default;
    
    // Reserve space for better performance
    void reserve(size_t capacity) {
        index_map.reserve(capacity);
        std::apply([capacity](auto&... vectors) {
            ((vectors.reserve(capacity / num_types)), ...);
        }, type_vectors);
    }
    
    // Add element of specific type
    template<typename T>
    void push_back(T&& value) {
        static_assert((std::is_same_v<std::decay_t<T>, Types> || ...));
        
        constexpr size_t type_idx = []() {
            size_t idx = 0;
            ((std::is_same_v<std::decay_t<T>, Types> ? true : (++idx, false)) || ...);
            return idx;
        }();
        
        auto& vec = std::get<type_idx>(type_vectors);
        vec.push_back(std::forward<T>(value));
        index_map.emplace_back(type_idx, vec.size() - 1);
    }
    
    // Emplace element of specific type
    template<typename T, typename... Args>
    void emplace_back(Args&&... args) {
        static_assert((std::is_same_v<T, Types> || ...));
        
        constexpr size_t type_idx = []() {
            size_t idx = 0;
            ((std::is_same_v<T, Types> ? true : (++idx, false)) || ...);
            return idx;
        }();
        
        auto& vec = std::get<type_idx>(type_vectors);
        vec.emplace_back(std::forward<Args>(args)...);
        index_map.emplace_back(type_idx, vec.size() - 1);
    }
    
    // Access element as variant
    variant_type at(size_t global_idx) const {
        if (global_idx >= index_map.size()) {
            throw std::out_of_range("Index out of range");
        }
        
        auto [type_idx, local_idx] = index_map[global_idx];
        return get_variant_at(type_idx, local_idx);
    }
    
    variant_type operator[](size_t global_idx) const {
        auto [type_idx, local_idx] = index_map[global_idx];
        return get_variant_at(type_idx, local_idx);
    }
    
    // Direct typed access (faster, no variant overhead)
    template<typename T>
    const T& get_typed(size_t global_idx) const {
        auto [type_idx, local_idx] = index_map[global_idx];
        constexpr size_t expected_type_idx = []() {
            size_t idx = 0;
            ((std::is_same_v<T, Types> ? true : (++idx, false)) || ...);
            return idx;
        }();
        
        if (type_idx != expected_type_idx) {
            throw std::bad_variant_access{};
        }
        
        return std::get<expected_type_idx>(type_vectors)[local_idx];
    }
    
    // Size and capacity
    size_t size() const { return index_map.size(); }
    bool empty() const { return index_map.empty(); }
    
    // Memory usage statistics
    size_t memory_usage() const {
        size_t total = index_map.capacity() * sizeof(std::pair<uint8_t, size_t>);
        std::apply([&total](const auto&... vectors) {
            ((total += vectors.capacity() * sizeof(typename std::decay_t<decltype(vectors)>::value_type)), ...);
        }, type_vectors);
        return total;
    }
    
    // Iterate over all elements of a specific type
    template<typename T>
    const std::vector<T>& get_type_vector() const {
        constexpr size_t type_idx = []() {
            size_t idx = 0;
            ((std::is_same_v<T, Types> ? true : (++idx, false)) || ...);
            return idx;
        }();
        return std::get<type_idx>(type_vectors);
    }
    
    // Get type index for global index
    uint8_t get_type_index(size_t global_idx) const {
        return index_map[global_idx].first;
    }
    
private:
    variant_type get_variant_at(uint8_t type_idx, size_t local_idx) const {
        return get_variant_helper(type_idx, local_idx, std::make_index_sequence<num_types>{});
    }
    
    template<size_t... Is>
    variant_type get_variant_helper(uint8_t type_idx, size_t local_idx, std::index_sequence<Is...>) const {
        variant_type result;
        ((type_idx == Is ? (result = std::get<Is>(type_vectors)[local_idx], true) : false) || ...);
        return result;
    }
};

// ============================================================================
// DYNAMIC VARIANT VECTOR (Runtime flexibility with type erasure)
// ============================================================================

class dynamic_variant_vector {
private:
    struct type_storage_base {
        virtual ~type_storage_base() = default;
        virtual void* get_at(size_t idx) = 0;
        virtual const void* get_at(size_t idx) const = 0;
        virtual size_t size() const = 0;
        virtual size_t capacity() const = 0;
        virtual void reserve(size_t cap) = 0;
        virtual size_t element_size() const = 0;
        virtual size_t memory_usage() const = 0;
    };
    
    template<typename T>
    struct type_storage : type_storage_base {
        std::vector<T> data;
        
        void* get_at(size_t idx) override { return &data[idx]; }
        const void* get_at(size_t idx) const override { return &data[idx]; }
        size_t size() const override { return data.size(); }
        size_t capacity() const override { return data.capacity(); }
        void reserve(size_t cap) override { data.reserve(cap); }
        size_t element_size() const override { return sizeof(T); }
        size_t memory_usage() const override { return data.capacity() * sizeof(T); }
        
        size_t push_back(T&& value) {
            data.push_back(std::move(value));
            return data.size() - 1;
        }
        
        template<typename... Args>
        size_t emplace_back(Args&&... args) {
            data.emplace_back(std::forward<Args>(args)...);
            return data.size() - 1;
        }
    };
    
    // Type registry
    std::vector<std::unique_ptr<type_storage_base>> type_storages;
    std::unordered_map<std::type_index, size_t> type_to_storage_idx;
    
    // Global index mapping
    std::vector<std::pair<size_t, size_t>> index_map; // (storage_idx, local_idx)
    
public:
    dynamic_variant_vector() = default;
    
    template<typename T>
    void register_type() {
        auto type_idx = std::type_index(typeid(T));
        if (type_to_storage_idx.find(type_idx) == type_to_storage_idx.end()) {
            type_to_storage_idx[type_idx] = type_storages.size();
            type_storages.push_back(std::make_unique<type_storage<T>>());
        }
    }
    
    template<typename T>
    void push_back(T&& value) {
        register_type<std::decay_t<T>>();
        auto type_idx = std::type_index(typeid(std::decay_t<T>));
        auto storage_idx = type_to_storage_idx[type_idx];
        
        auto* storage = static_cast<type_storage<std::decay_t<T>>*>(type_storages[storage_idx].get());
        auto local_idx = storage->push_back(std::forward<T>(value));
        index_map.emplace_back(storage_idx, local_idx);
    }
    
    template<typename T, typename... Args>
    void emplace_back(Args&&... args) {
        register_type<T>();
        auto type_idx = std::type_index(typeid(T));
        auto storage_idx = type_to_storage_idx[type_idx];
        
        auto* storage = static_cast<type_storage<T>*>(type_storages[storage_idx].get());
        auto local_idx = storage->template emplace_back<Args...>(std::forward<Args>(args)...);
        index_map.emplace_back(storage_idx, local_idx);
    }
    
    template<typename T>
    const T& get_typed(size_t global_idx) const {
        auto [storage_idx, local_idx] = index_map[global_idx];
        auto* storage = static_cast<const type_storage<T>*>(type_storages[storage_idx].get());
        return *static_cast<const T*>(storage->get_at(local_idx));
    }
    
    template<typename T>
    T& get_typed(size_t global_idx) {
        auto [storage_idx, local_idx] = index_map[global_idx];
        auto* storage = static_cast<type_storage<T>*>(type_storages[storage_idx].get());
        return *static_cast<T*>(storage->get_at(local_idx));
    }
    
    size_t size() const { return index_map.size(); }
    bool empty() const { return index_map.empty(); }
    
    void reserve(size_t capacity) {
        index_map.reserve(capacity);
        for (auto& storage : type_storages) {
            storage->reserve(capacity / type_storages.size());
        }
    }
    
    size_t memory_usage() const {
        size_t total = index_map.capacity() * sizeof(std::pair<size_t, size_t>);
        for (const auto& storage : type_storages) {
            total += storage->memory_usage();
        }
        return total;
    }
    
    // Get all elements of a specific type
    template<typename T>
    const std::vector<T>& get_type_vector() const {
        auto type_idx = std::type_index(typeid(T));
        auto it = type_to_storage_idx.find(type_idx);
        if (it == type_to_storage_idx.end()) {
            throw std::runtime_error("Type not registered");
        }
        
        auto* storage = static_cast<const type_storage<T>*>(type_storages[it->second].get());
        return storage->data;
    }
};

