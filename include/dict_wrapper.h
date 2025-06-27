#ifndef DICT_WRAPPER_H
#define DICT_WRAPPER_H

#include <unordered_map>
#include <utility> // For std::pair, std::move

template <typename K, typename V, typename InnerMap = std::unordered_map<K, V>>
class DictWrapper {
public:
  using key_type = K;
  using mapped_type = V;
  using value_type = typename InnerMap::value_type; // std::pair<const K, V>
  using size_type = typename InnerMap::size_type;
  using difference_type = typename InnerMap::difference_type;
  using hasher = typename InnerMap::hasher;
  using key_equal = typename InnerMap::key_equal;
  using allocator_type = typename InnerMap::allocator_type;
  using reference = typename InnerMap::reference;
  using const_reference = typename InnerMap::const_reference;
  using pointer = typename InnerMap::pointer;
  using const_pointer = typename InnerMap::const_pointer;
  using iterator = typename InnerMap::iterator;
  using const_iterator = typename InnerMap::const_iterator;

protected:
  InnerMap data;

public:
  // Constructors
  DictWrapper() = default;
  explicit DictWrapper(const InnerMap &d) : data(d) {}
  explicit DictWrapper(InnerMap &&d) : data(std::move(d)) {}

  template <typename InputIt>
  DictWrapper(InputIt first, InputIt last) : data(first, last) {}

  DictWrapper(std::initializer_list<value_type> il) : data(il) {}

  // Copy constructor
  DictWrapper(const DictWrapper &other) : data(other.data) {}

  // Move constructor
  DictWrapper(DictWrapper &&other) noexcept : data(std::move(other.data)) {}

  // Assignment operators
  DictWrapper &operator=(const DictWrapper &other) {
    if (this != &other) {
      data = other.data;
    }
    return *this;
  }

  DictWrapper &operator=(DictWrapper &&other) noexcept {
    if (this != &other) {
      data = std::move(other.data);
    }
    return *this;
  }

  DictWrapper &operator=(std::initializer_list<value_type> il) {
    data = il;
    return *this;
  }

  // Element access
  virtual mapped_type &operator[](const key_type &key) { return data[key]; }

  virtual mapped_type &operator[](key_type &&key) {
    return data[std::move(key)];
  }

  virtual mapped_type &at(const key_type &key) { return data.at(key); }

  virtual const mapped_type &at(const key_type &key) const {
    return data.at(key);
  }

  // Iterators
  virtual iterator begin() noexcept { return data.begin(); }

  virtual const_iterator begin() const noexcept { return data.begin(); }

  virtual const_iterator cbegin() const noexcept { return data.cbegin(); }

  virtual iterator end() noexcept { return data.end(); }

  virtual const_iterator end() const noexcept { return data.end(); }

  virtual const_iterator cend() const noexcept { return data.cend(); }

  // Capacity
  virtual bool empty() const noexcept { return data.empty(); }

  virtual size_type size() const noexcept { return data.size(); }

  virtual size_type max_size() const noexcept { return data.max_size(); }

  // Modifiers
  virtual void clear() noexcept { data.clear(); }

  virtual std::pair<iterator, bool> insert(const value_type &value) {
    return data.insert(value);
  }

  virtual std::pair<iterator, bool> insert(value_type &&value) {
    return data.insert(std::move(value));
  }

  template <typename P> std::pair<iterator, bool> insert(P &&value) {
    return data.insert(std::forward<P>(value));
  }

  iterator insert(const_iterator hint, const value_type &value) {
    return data.insert(hint, value);
  }

  iterator insert(const_iterator hint, value_type &&value) {
    return data.insert(hint, std::move(value));
  }

  template <typename P> iterator insert(const_iterator hint, P &&value) {
    return data.insert(hint, std::forward<P>(value));
  }

  template <typename InputIt> void insert(InputIt first, InputIt last) {
    data.insert(first, last);
  }

  void insert(std::initializer_list<value_type> il) { data.insert(il); }

  template <typename... Args>
  std::pair<iterator, bool> emplace(Args &&...args) {
    return data.emplace(std::forward<Args>(args)...);
  }

  template <typename... Args>
  iterator emplace_hint(const_iterator hint, Args &&...args) {
    return data.emplace_hint(hint, std::forward<Args>(args)...);
  }

  virtual iterator erase(const_iterator pos) { return data.erase(pos); }

  virtual iterator erase(const_iterator first, const_iterator last) {
    return data.erase(first, last);
  }

  virtual size_type erase(const key_type &key) { return data.erase(key); }

  virtual void swap(DictWrapper &other) noexcept { data.swap(other.data); }

  // Lookup
  virtual size_type count(const key_type &key) const { return data.count(key); }

  virtual iterator find(const key_type &key) { return data.find(key); }

  virtual const_iterator find(const key_type &key) const {
    return data.find(key);
  }

  virtual bool contains(const key_type &key) const {
    if constexpr (requires { data.contains(key); }) { // C++20 feature
      return data.contains(key);
    } else {
      return data.find(key) != data.end();
    }
  }

  virtual std::pair<iterator, iterator> equal_range(const key_type &key) {
    return data.equal_range(key);
  }

  virtual std::pair<const_iterator, const_iterator>
  equal_range(const key_type &key) const {
    return data.equal_range(key);
  }

  // Bucket interface (forwarding as is)
  // These are less commonly overridden but provided for completeness
  virtual float load_factor() const { return data.load_factor(); }

  virtual float max_load_factor() const { return data.max_load_factor(); }

  virtual void max_load_factor(float ml) { data.max_load_factor(ml); }

  virtual void rehash(size_type count) { data.rehash(count); }

  virtual void reserve(size_type count) { data.reserve(count); }

  // Hash policy
  virtual hasher hash_function() const { return data.hash_function(); }

  virtual key_equal key_eq() const { return data.key_eq(); }

  // Allow access to underlying data for derived classes if needed, though
  // direct access is discouraged in favor of overriding methods.
protected:
  InnerMap &get_data() { return data; }
  const InnerMap &get_data() const { return data; }
};

// Non-member functions
template <typename K, typename V, typename InnerMap>
void swap(DictWrapper<K, V, InnerMap> &lhs,
          DictWrapper<K, V, InnerMap> &rhs) noexcept {
  lhs.swap(rhs);
}

template <typename K, typename V, typename InnerMap>
bool operator==(const DictWrapper<K, V, InnerMap> &lhs,
                const DictWrapper<K, V, InnerMap> &rhs) {
  // Access protected data for comparison. This requires DictWrapper to be a
  // friend or to expose data via a protected getter. Using a getter is cleaner.
  // Alternatively, iterate and compare, but direct map comparison is more
  // efficient. For now, let's assume we need a way to compare underlying maps.
  // If InnerMap itself doesn't support operator==, this will fail.
  // std::unordered_map supports operator==.
  // This comparison should ideally be virtual if derived classes change
  // equality definition. However, for a simple wrapper, comparing underlying
  // data is standard. Let's stick to comparing the protected 'data' member.
  // This would require friend declaration or making 'data' accessible.
  // A simpler approach for now, not requiring friend:
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (const auto &p : lhs) {
    auto it = rhs.find(p.first);
    if (it == rhs.end() || !(it->second == p.second)) {
      return false;
    }
  }
  return true;
}

template <typename K, typename V, typename InnerMap>
bool operator!=(const DictWrapper<K, V, InnerMap> &lhs,
                const DictWrapper<K, V, InnerMap> &rhs) {
  return !(lhs == rhs);
}

#endif // DICT_WRAPPER_H
