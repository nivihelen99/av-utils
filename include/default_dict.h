#pragma once

#include <unordered_map>
#include <functional>
#include <type_traits>
#include <utility>
#include <memory>

namespace std_ext {

template<
    typename Key,
    typename Value,
    typename DefaultFactory = std::function<Value()>,
    typename Hash = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>,
    typename Allocator = std::allocator<std::pair<const Key, Value>>
>
class defaultdict {
public:
    // STL-compatible type aliases
    using key_type = Key;
    using mapped_type = Value;
    using value_type = std::pair<const Key, Value>;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using hasher = Hash;
    using key_equal = KeyEqual;
    using allocator_type = Allocator;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = typename std::allocator_traits<Allocator>::pointer;
    using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;

private:
    using container_type = std::unordered_map<Key, Value, Hash, KeyEqual, Allocator>;
    
public:
    using iterator = typename container_type::iterator;
    using const_iterator = typename container_type::const_iterator;
    using local_iterator = typename container_type::local_iterator;
    using const_local_iterator = typename container_type::const_local_iterator;

private:
    container_type container_;
    DefaultFactory default_factory_;

    // Helper to check if DefaultFactory is callable
    template<typename F>
    static constexpr bool is_callable_v = std::is_invocable_r_v<Value, F>;

public:
    // Constructors

    // Default constructor
    defaultdict() 
        requires std::default_initializable<DefaultFactory>
        : container_(), default_factory_() {}

    // Constructor with custom default factory
    template<typename F>
    explicit defaultdict(F&& factory)
        requires is_callable_v<std::decay_t<F>>
        : container_(), default_factory_(std::forward<F>(factory)) {}

    // Constructor with allocator
    explicit defaultdict(const Allocator& alloc)
        requires std::default_initializable<DefaultFactory>
        : container_(alloc), default_factory_() {}

    // Constructor with factory and allocator
    template<typename F>
    defaultdict(F&& factory, const Allocator& alloc)
        requires is_callable_v<std::decay_t<F>>
        : container_(alloc), default_factory_(std::forward<F>(factory)) {}

    // Constructor with bucket count
    explicit defaultdict(size_type bucket_count,
                        const Hash& hash = Hash(),
                        const KeyEqual& equal = KeyEqual(),
                        const Allocator& alloc = Allocator())
        requires std::default_initializable<DefaultFactory>
        : container_(bucket_count, hash, equal, alloc), default_factory_() {}

    // Constructor with factory and bucket count
    template<typename F>
    defaultdict(F&& factory,
               size_type bucket_count,
               const Hash& hash = Hash(),
               const KeyEqual& equal = KeyEqual(),
               const Allocator& alloc = Allocator())
        requires is_callable_v<std::decay_t<F>>
        : container_(bucket_count, hash, equal, alloc), 
          default_factory_(std::forward<F>(factory)) {}

    // Range constructor
    template<typename InputIt>
    defaultdict(InputIt first, InputIt last,
               size_type bucket_count = 0,
               const Hash& hash = Hash(),
               const KeyEqual& equal = KeyEqual(),
               const Allocator& alloc = Allocator())
        requires std::default_initializable<DefaultFactory>
        : container_(first, last, bucket_count, hash, equal, alloc), default_factory_() {}

    // Range constructor with factory
    template<typename InputIt, typename F>
    defaultdict(F&& factory,
               InputIt first, InputIt last,
               size_type bucket_count = 0,
               const Hash& hash = Hash(),
               const KeyEqual& equal = KeyEqual(),
               const Allocator& alloc = Allocator())
        requires is_callable_v<std::decay_t<F>>
        : container_(first, last, bucket_count, hash, equal, alloc),
          default_factory_(std::forward<F>(factory)) {}

    // Initializer list constructor
    defaultdict(std::initializer_list<value_type> init,
               size_type bucket_count = 0,
               const Hash& hash = Hash(),
               const KeyEqual& equal = KeyEqual(),
               const Allocator& alloc = Allocator())
        requires std::default_initializable<DefaultFactory>
        : container_(init, bucket_count, hash, equal, alloc), default_factory_() {}

    // Initializer list constructor with factory
    template<typename F>
    defaultdict(F&& factory,
               std::initializer_list<value_type> init,
               size_type bucket_count = 0,
               const Hash& hash = Hash(),
               const KeyEqual& equal = KeyEqual(),
               const Allocator& alloc = Allocator())
        requires is_callable_v<std::decay_t<F>>
        : container_(init, bucket_count, hash, equal, alloc),
          default_factory_(std::forward<F>(factory)) {}

    // Copy constructor
    defaultdict(const defaultdict& other) = default;

    // Copy constructor with allocator
    defaultdict(const defaultdict& other, const Allocator& alloc)
        : container_(other.container_, alloc), default_factory_(other.default_factory_) {}

    // Move constructor
    defaultdict(defaultdict&& other) noexcept = default;

    // Move constructor with allocator
    defaultdict(defaultdict&& other, const Allocator& alloc)
        : container_(std::move(other.container_), alloc),
          default_factory_(std::move(other.default_factory_)) {}

    // Assignment operators
    defaultdict& operator=(const defaultdict& other) = default;
    defaultdict& operator=(defaultdict&& other) noexcept = default;
    defaultdict& operator=(std::initializer_list<value_type> init) {
        container_ = init;
        return *this;
    }

    // Destructor
    ~defaultdict() = default;

    // Element access with automatic default insertion
    Value& operator[](const Key& key) {
        auto it = container_.find(key);
        if (it != container_.end()) {
            return it->second;
        }
        return container_.emplace(key, default_factory_()).first->second;
    }

    Value& operator[](Key&& key) {
        auto it = container_.find(key);
        if (it != container_.end()) {
            return it->second;
        }
        return container_.emplace(std::move(key), default_factory_()).first->second;
    }

    // at() method - throws if key doesn't exist (no default insertion)
    Value& at(const Key& key) {
        return container_.at(key);
    }

    const Value& at(const Key& key) const {
        return container_.at(key);
    }

    // Iterators
    iterator begin() noexcept { return container_.begin(); }
    const_iterator begin() const noexcept { return container_.begin(); }
    const_iterator cbegin() const noexcept { return container_.cbegin(); }

    iterator end() noexcept { return container_.end(); }
    const_iterator end() const noexcept { return container_.end(); }
    const_iterator cend() const noexcept { return container_.cend(); }

    // Capacity
    bool empty() const noexcept { return container_.empty(); }
    size_type size() const noexcept { return container_.size(); }
    size_type max_size() const noexcept { return container_.max_size(); }

    // Modifiers
    void clear() noexcept { container_.clear(); }

    std::pair<iterator, bool> insert(const value_type& value) {
        return container_.insert(value);
    }

    std::pair<iterator, bool> insert(value_type&& value) {
        return container_.insert(std::move(value));
    }

    template<typename P>
    std::pair<iterator, bool> insert(P&& value)
        requires std::constructible_from<value_type, P&&>
    {
        return container_.insert(std::forward<P>(value));
    }

    iterator insert(const_iterator hint, const value_type& value) {
        return container_.insert(hint, value);
    }

    iterator insert(const_iterator hint, value_type&& value) {
        return container_.insert(hint, std::move(value));
    }

    template<typename P>
    iterator insert(const_iterator hint, P&& value)
        requires std::constructible_from<value_type, P&&>
    {
        return container_.insert(hint, std::forward<P>(value));
    }

    template<typename InputIt>
    void insert(InputIt first, InputIt last) {
        container_.insert(first, last);
    }

    void insert(std::initializer_list<value_type> init) {
        container_.insert(init);
    }

    template<typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        return container_.emplace(std::forward<Args>(args)...);
    }

    template<typename... Args>
    iterator emplace_hint(const_iterator hint, Args&&... args) {
        return container_.emplace_hint(hint, std::forward<Args>(args)...);
    }

    template<typename... Args>
    std::pair<iterator, bool> try_emplace(const Key& key, Args&&... args) {
        return container_.try_emplace(key, std::forward<Args>(args)...);
    }

    template<typename... Args>
    std::pair<iterator, bool> try_emplace(Key&& key, Args&&... args) {
        return container_.try_emplace(std::move(key), std::forward<Args>(args)...);
    }

    template<typename... Args>
    iterator try_emplace(const_iterator hint, const Key& key, Args&&... args) {
        return container_.try_emplace(hint, key, std::forward<Args>(args)...);
    }

    template<typename... Args>
    iterator try_emplace(const_iterator hint, Key&& key, Args&&... args) {
        return container_.try_emplace(hint, std::move(key), std::forward<Args>(args)...);
    }

    template<typename M>
    std::pair<iterator, bool> insert_or_assign(const Key& key, M&& obj) {
        return container_.insert_or_assign(key, std::forward<M>(obj));
    }

    template<typename M>
    std::pair<iterator, bool> insert_or_assign(Key&& key, M&& obj) {
        return container_.insert_or_assign(std::move(key), std::forward<M>(obj));
    }

    template<typename M>
    iterator insert_or_assign(const_iterator hint, const Key& key, M&& obj) {
        return container_.insert_or_assign(hint, key, std::forward<M>(obj));
    }

    template<typename M>
    iterator insert_or_assign(const_iterator hint, Key&& key, M&& obj) {
        return container_.insert_or_assign(hint, std::move(key), std::forward<M>(obj));
    }

    iterator erase(iterator pos) {
        return container_.erase(pos);
    }

    iterator erase(const_iterator pos) {
        return container_.erase(pos);
    }

    iterator erase(const_iterator first, const_iterator last) {
        return container_.erase(first, last);
    }

    size_type erase(const Key& key) {
        return container_.erase(key);
    }

    template<typename K>
    size_type erase(K&& key)
        requires requires { container_.erase(std::forward<K>(key)); }
    {
        return container_.erase(std::forward<K>(key));
    }

    void swap(defaultdict& other) noexcept(
        std::is_nothrow_swappable_v<container_type> &&
        std::is_nothrow_swappable_v<DefaultFactory>)
    {
        using std::swap;
        swap(container_, other.container_);
        swap(default_factory_, other.default_factory_);
    }

    // Lookup
    size_type count(const Key& key) const {
        return container_.count(key);
    }

    template<typename K>
    size_type count(const K& key) const
        requires requires { container_.count(key); }
    {
        return container_.count(key);
    }

    iterator find(const Key& key) {
        return container_.find(key);
    }

    const_iterator find(const Key& key) const {
        return container_.find(key);
    }

    template<typename K>
    iterator find(const K& key)
        requires requires { container_.find(key); }
    {
        return container_.find(key);
    }

    template<typename K>
    const_iterator find(const K& key) const
        requires requires { container_.find(key); }
    {
        return container_.find(key);
    }

    bool contains(const Key& key) const {
        return container_.contains(key);
    }

    template<typename K>
    bool contains(const K& key) const
        requires requires { container_.contains(key); }
    {
        return container_.contains(key);
    }

    std::pair<iterator, iterator> equal_range(const Key& key) {
        return container_.equal_range(key);
    }

    std::pair<const_iterator, const_iterator> equal_range(const Key& key) const {
        return container_.equal_range(key);
    }

    template<typename K>
    std::pair<iterator, iterator> equal_range(const K& key)
        requires requires { container_.equal_range(key); }
    {
        return container_.equal_range(key);
    }

    template<typename K>
    std::pair<const_iterator, const_iterator> equal_range(const K& key) const
        requires requires { container_.equal_range(key); }
    {
        return container_.equal_range(key);
    }

    // Bucket interface
    local_iterator begin(size_type n) { return container_.begin(n); }
    const_local_iterator begin(size_type n) const { return container_.begin(n); }
    const_local_iterator cbegin(size_type n) const { return container_.cbegin(n); }

    local_iterator end(size_type n) { return container_.end(n); }
    const_local_iterator end(size_type n) const { return container_.end(n); }
    const_local_iterator cend(size_type n) const { return container_.cend(n); }

    size_type bucket_count() const { return container_.bucket_count(); }
    size_type max_bucket_count() const { return container_.max_bucket_count(); }
    size_type bucket_size(size_type n) const { return container_.bucket_size(n); }
    size_type bucket(const Key& key) const { return container_.bucket(key); }

    // Hash policy
    float load_factor() const { return container_.load_factor(); }
    float max_load_factor() const { return container_.max_load_factor(); }
    void max_load_factor(float ml) { container_.max_load_factor(ml); }

    void rehash(size_type count) { container_.rehash(count); }
    void reserve(size_type count) { container_.reserve(count); }

    // Observers
    hasher hash_function() const { return container_.hash_function(); }
    key_equal key_eq() const { return container_.key_eq(); }

    // Allocator
    allocator_type get_allocator() const { return container_.get_allocator(); }

    // Access to default factory
    const DefaultFactory& get_default_factory() const noexcept {
        return default_factory_;
    }

    // Set new default factory
    template<typename F>
    void set_default_factory(F&& factory)
        requires is_callable_v<std::decay_t<F>>
    {
        default_factory_ = std::forward<F>(factory);
    }

    // Get with default (doesn't modify container)
    Value get(const Key& key) const {
        auto it = container_.find(key);
        return (it != container_.end()) ? it->second : default_factory_();
    }

    template<typename K>
    Value get(const K& key) const
        requires requires { container_.find(key); }
    {
        auto it = container_.find(key);
        return (it != container_.end()) ? it->second : default_factory_();
    }

    // Convenience method for setting defaults for multiple keys
    template<typename InputIt>
    void set_defaults(InputIt first, InputIt last) {
        for (auto it = first; it != last; ++it) {
            (*this)[*it]; // This will create default value if key doesn't exist
        }
    }

    void set_defaults(std::initializer_list<Key> keys) {
        set_defaults(keys.begin(), keys.end());
    }
};

// Non-member functions

// Comparison operators
template<typename Key, typename Value, typename DF1, typename Hash, typename KeyEqual, typename Alloc>
bool operator==(const defaultdict<Key, Value, DF1, Hash, KeyEqual, Alloc>& lhs,
                const defaultdict<Key, Value, DF1, Hash, KeyEqual, Alloc>& rhs) {
    return lhs.size() == rhs.size() && 
           std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

template<typename Key, typename Value, typename DF1, typename Hash, typename KeyEqual, typename Alloc>
bool operator!=(const defaultdict<Key, Value, DF1, Hash, KeyEqual, Alloc>& lhs,
                const defaultdict<Key, Value, DF1, Hash, KeyEqual, Alloc>& rhs) {
    return !(lhs == rhs);
}

// Swap
template<typename Key, typename Value, typename DF, typename Hash, typename KeyEqual, typename Alloc>
void swap(defaultdict<Key, Value, DF, Hash, KeyEqual, Alloc>& lhs,
          defaultdict<Key, Value, DF, Hash, KeyEqual, Alloc>& rhs)
    noexcept(noexcept(lhs.swap(rhs)))
{
    lhs.swap(rhs);
}

// Deduction guides
template<typename InputIt>
defaultdict(InputIt, InputIt) -> defaultdict<
    typename std::iterator_traits<InputIt>::value_type::first_type,
    typename std::iterator_traits<InputIt>::value_type::second_type
>;

template<typename Key, typename Value>
defaultdict(std::initializer_list<std::pair<Key, Value>>) -> defaultdict<Key, Value>;

template<typename F, typename InputIt>
defaultdict(F, InputIt, InputIt) -> defaultdict<
    typename std::iterator_traits<InputIt>::value_type::first_type,
    typename std::iterator_traits<InputIt>::value_type::second_type,
    std::decay_t<F>
>;

template<typename F, typename Key, typename Value>
defaultdict(F, std::initializer_list<std::pair<Key, Value>>) -> defaultdict<Key, Value, std::decay_t<F>>;

// Factory function helpers
template<typename Value>
auto default_factory() {
    if constexpr (std::default_initializable<Value>) {
        return []() -> Value { return Value{}; };
    } else {
        static_assert(std::default_initializable<Value>, 
                     "Value type must be default constructible for default factory");
    }
}

template<typename Value>
auto zero_factory() {
    static_assert(std::constructible_from<Value, int>, 
                 "Value type must be constructible from int for zero factory");
    return []() -> Value { return Value{0}; };
}

template<typename Value>
auto list_factory() {
    static_assert(std::default_initializable<Value>, 
                 "Value type must be default constructible for list factory");
    return []() -> Value { return Value{}; };
}

} // namespace std_ext

