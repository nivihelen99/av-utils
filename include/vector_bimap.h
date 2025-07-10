#pragma once

#include <vector>
#include <utility>      // For std::pair, std::move
#include <algorithm>    // For std::lower_bound, std::sort, std::find_if
#include <functional>   // For std::less
#include <stdexcept>    // For std::out_of_range
#include <initializer_list>

namespace cpp_collections {

template<typename Left, typename Right,
         typename CompareLeft = std::less<Left>,
         typename CompareRight = std::less<Right>>
class VectorBiMap {
public:
    using left_type = Left;
    using right_type = Right;
    using left_value_type = std::pair<Left, Right>;
    using right_value_type = std::pair<Right, Left>;

    // For compatibility with map-like interfaces, 'key_type' and 'mapped_type'
    // usually refer to one primary direction. We'll alias them for the left-to-right view.
    using key_type = Left;
    using mapped_type = Right;
    using value_type = left_value_type; // std::pair<const Key, Value> for map compatibility

    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    using compare_left = CompareLeft;
    using compare_right = CompareRight;

private:
    // Comparators for std::pair based on the first element
    struct LeftPairCompare {
        CompareLeft comp;
        LeftPairCompare(const CompareLeft& c = CompareLeft()) : comp(c) {}
        bool operator()(const left_value_type& a, const left_value_type& b) const {
            return comp(a.first, b.first);
        }
        bool operator()(const left_value_type& a, const Left& b_key) const {
            return comp(a.first, b_key);
        }
        bool operator()(const Left& a_key, const left_value_type& b) const {
            return comp(a_key, b.first);
        }
    };

    struct RightPairCompare {
        CompareRight comp;
        RightPairCompare(const CompareRight& c = CompareRight()) : comp(c) {}
        bool operator()(const right_value_type& a, const right_value_type& b) const {
            return comp(a.first, b.first);
        }
        bool operator()(const right_value_type& a, const Right& b_key) const {
            return comp(a.first, b_key);
        }
        bool operator()(const Right& a_key, const right_value_type& b) const {
            return comp(a_key, b.first);
        }
    };

    std::vector<left_value_type> left_to_right_data_;
    std::vector<right_value_type> right_to_left_data_;
    CompareLeft compare_left_fn_;
    CompareRight compare_right_fn_;
    LeftPairCompare left_pair_compare_;
    RightPairCompare right_pair_compare_;

public:
    // Constructors
    VectorBiMap()
        : compare_left_fn_(CompareLeft()), compare_right_fn_(CompareRight()),
          left_pair_compare_(compare_left_fn_), right_pair_compare_(compare_right_fn_) {}

    explicit VectorBiMap(const CompareLeft& comp_left, const CompareRight& comp_right = CompareRight())
        : compare_left_fn_(comp_left), compare_right_fn_(comp_right),
          left_pair_compare_(comp_left), right_pair_compare_(comp_right) {}

    VectorBiMap(std::initializer_list<left_value_type> init,
                const CompareLeft& comp_left = CompareLeft(),
                const CompareRight& comp_right = CompareRight())
        : compare_left_fn_(comp_left), compare_right_fn_(comp_right),
          left_pair_compare_(comp_left), right_pair_compare_(comp_right) {
        left_to_right_data_.reserve(init.size());
        right_to_left_data_.reserve(init.size());
        for (const auto& pair : init) {
            // Simple push_back for now; will be sorted later or use insert.
            // For constructor, it's better to sort at the end.
            left_to_right_data_.push_back(pair);
            right_to_left_data_.emplace_back(pair.second, pair.first);
        }
        std::sort(left_to_right_data_.begin(), left_to_right_data_.end(), left_pair_compare_);
        std::sort(right_to_left_data_.begin(), right_to_left_data_.end(), right_pair_compare_);
        // TODO: Add check for duplicate keys/values in initializer_list and handle appropriately
        // (e.g., throw, or keep first/last encountered). For now, assumes valid unique pairs.
    }

    // Rule of 5
    VectorBiMap(const VectorBiMap& other) = default;
    VectorBiMap(VectorBiMap&& other) noexcept = default;
    VectorBiMap& operator=(const VectorBiMap& other) = default;
    VectorBiMap& operator=(VectorBiMap&& other) noexcept = default;
    ~VectorBiMap() = default;

    // Iterators (to be defined later)
    // class left_iterator;
    // class const_left_iterator;
    // class right_iterator;
    // class const_right_iterator;

    // Basic capacity methods (will be filled in later)
    size_type size() const noexcept { return left_to_right_data_.size(); }
    bool empty() const noexcept { return left_to_right_data_.empty(); }
    void clear() noexcept {
        left_to_right_data_.clear();
        right_to_left_data_.clear();
    }

    void swap(VectorBiMap& other) noexcept {
        using std::swap;
        swap(left_to_right_data_, other.left_to_right_data_);
        swap(right_to_left_data_, other.right_to_left_data_);
        swap(compare_left_fn_, other.compare_left_fn_);
        swap(compare_right_fn_, other.compare_right_fn_);
        // Re-initialize pair comparators if they hold references to the function objects
        // or if the function objects themselves were swapped.
        // If CompareLeft/Right are simple function objects (like std::less),
        // swapping them is enough. If they have state or are complex, this might need adjustment.
        // For now, assuming simple swappable comparators.
        swap(left_pair_compare_, other.left_pair_compare_);
        swap(right_pair_compare_, other.right_pair_compare_);
    }

    // Comparators access
    CompareLeft left_key_comp() const { return compare_left_fn_; }
    CompareRight right_key_comp() const { return compare_right_fn_; }

    // --- Lookup Functions ---

    // Returns iterator to element in left-to-right view, or end iterator if not found.
    typename std::vector<left_value_type>::const_iterator find_left_iter(const Left& key) const {
        auto it = std::lower_bound(left_to_right_data_.begin(), left_to_right_data_.end(), key, left_pair_compare_);
        if (it != left_to_right_data_.end() && !left_pair_compare_(key, *it)) { // Key is not less than it->first, so it might be equal
            return it;
        }
        return left_to_right_data_.end();
    }

    typename std::vector<left_value_type>::iterator find_left_iter(const Left& key) {
        auto it = std::lower_bound(left_to_right_data_.begin(), left_to_right_data_.end(), key, left_pair_compare_);
        if (it != left_to_right_data_.end() && !left_pair_compare_(key, *it)) {
            return it;
        }
        return left_to_right_data_.end();
    }

    // Returns iterator to element in right-to-left view, or end iterator if not found.
    typename std::vector<right_value_type>::const_iterator find_right_iter(const Right& key) const {
        auto it = std::lower_bound(right_to_left_data_.begin(), right_to_left_data_.end(), key, right_pair_compare_);
        if (it != right_to_left_data_.end() && !right_pair_compare_(key, *it)) {
            return it;
        }
        return right_to_left_data_.end();
    }

    typename std::vector<right_value_type>::iterator find_right_iter(const Right& key) {
        auto it = std::lower_bound(right_to_left_data_.begin(), right_to_left_data_.end(), key, right_pair_compare_);
        if (it != right_to_left_data_.end() && !right_pair_compare_(key, *it)) {
            return it;
        }
        return right_to_left_data_.end();
    }

    const Right* find_left(const Left& key) const {
        auto it = find_left_iter(key);
        return (it != left_to_right_data_.end()) ? &(it->second) : nullptr;
    }

    Right* find_left(const Left& key) {
        auto it = find_left_iter(key);
        return (it != left_to_right_data_.end()) ? &(it->second) : nullptr;
    }

    const Left* find_right(const Right& key) const {
        auto it = find_right_iter(key);
        return (it != right_to_left_data_.end()) ? &(it->second) : nullptr;
    }

    Left* find_right(const Right& key) {
        auto it = find_right_iter(key);
        return (it != right_to_left_data_.end()) ? &(it->second) : nullptr;
    }

    bool contains_left(const Left& key) const {
        return find_left_iter(key) != left_to_right_data_.end();
    }

    bool contains_right(const Right& key) const {
        return find_right_iter(key) != right_to_left_data_.end();
    }

    const Right& at_left(const Left& key) const {
        const Right* val_ptr = find_left(key);
        if (!val_ptr) {
            throw std::out_of_range("VectorBiMap::at_left: key not found");
        }
        return *val_ptr;
    }

    Right& at_left(const Left& key) {
        Right* val_ptr = find_left(key);
        if (!val_ptr) {
            throw std::out_of_range("VectorBiMap::at_left: key not found");
        }
        return *val_ptr;
    }

    const Left& at_right(const Right& key) const {
        const Left* val_ptr = find_right(key);
        if (!val_ptr) {
            throw std::out_of_range("VectorBiMap::at_right: key not found");
        }
        return *val_ptr;
    }

    Left& at_right(const Right& key) {
        Left* val_ptr = find_right(key);
        if (!val_ptr) {
            throw std::out_of_range("VectorBiMap::at_right: key not found");
        }
        return *val_ptr;
    }

    // --- Modifiers ---

    // Inserts a (left, right) pair. Returns true if successful, false if either key already exists.
    bool insert(const Left& left_key, const Right& right_key) {
        if (contains_left(left_key) || contains_right(right_key)) {
            return false; // One of the keys already exists, cannot insert.
        }

        // Find insertion point for left_to_right_data_
        auto it_l = std::lower_bound(left_to_right_data_.begin(), left_to_right_data_.end(), left_key, left_pair_compare_);
        left_to_right_data_.emplace(it_l, left_key, right_key);

        // Find insertion point for right_to_left_data_
        auto it_r = std::lower_bound(right_to_left_data_.begin(), right_to_left_data_.end(), right_key, right_pair_compare_);
        right_to_left_data_.emplace(it_r, right_key, left_key);

        return true;
    }

    // Overload for rvalue keys
    bool insert(Left&& left_key, Right&& right_key) {
        // Need to check original values before they are potentially moved.
        // This is tricky if Left and Right are the same type and one is moved into the other.
        // For safety, copy if necessary or check carefully.
        // Let's assume for now that checking contains_left/right with potentially moved-from
        // values is okay if the types are distinct or if the check happens before any move.
        // A robust way is to check, then construct, then emplace.

        // Check with original values
        if (contains_left(left_key) || contains_right(right_key)) {
            return false;
        }

        // Store copies for the reverse mapping if types could alias or if one is moved before the other is used.
        Left l_copy = left_key;
        Right r_copy = right_key;

        auto it_l = std::lower_bound(left_to_right_data_.begin(), left_to_right_data_.end(), l_copy, left_pair_compare_);
        left_to_right_data_.emplace(it_l, std::move(left_key), std::move(right_key));

        auto it_r = std::lower_bound(right_to_left_data_.begin(), right_to_left_data_.end(), r_copy, right_pair_compare_);
        // Use copies for the reverse mapping as original left_key/right_key might have been moved.
        right_to_left_data_.emplace(it_r, std::move(r_copy), std::move(l_copy));

        return true;
    }

    // Insert or assign: if left_key exists, its mapping is updated.
    // If right_key exists and maps to a different left_key, that old mapping is removed.
    // Then, the new (left_key, right_key) mapping is established.
    void insert_or_assign(const Left& left_key, const Right& right_key) {
        // Try to find existing left_key
        auto it_l_old = find_left_iter(left_key);
        if (it_l_old != left_to_right_data_.end()) {
            // Left key exists. Get its old corresponding right_key.
            Right old_right_val = it_l_old->second;
            // If this old_right_val is different from the new right_key,
            // we need to remove the old_right_val from right_to_left_data_.
            if (!(compare_right_fn_(old_right_val, right_key) == false && compare_right_fn_(right_key, old_right_val) == false)) { // Not equal
                 auto it_r_for_old_right = find_right_iter(old_right_val);
                 if (it_r_for_old_right != right_to_left_data_.end()) {
                    right_to_left_data_.erase(it_r_for_old_right);
                 }
            }
            // Remove the old (left_key, old_right_val) from left_to_right_data_
            left_to_right_data_.erase(it_l_old);
        }

        // Try to find existing right_key
        auto it_r_old = find_right_iter(right_key);
        if (it_r_old != right_to_left_data_.end()) {
            // Right key exists. Get its old corresponding left_key.
            Left old_left_val = it_r_old->second;
            // If this old_left_val is different from the new left_key,
            // we need to remove the old_left_val from left_to_right_data_.
             if (!(compare_left_fn_(old_left_val, left_key) == false && compare_left_fn_(left_key, old_left_val) == false)) { // Not equal
                 auto it_l_for_old_left = find_left_iter(old_left_val);
                 if (it_l_for_old_left != left_to_right_data_.end()) {
                    left_to_right_data_.erase(it_l_for_old_left);
                 }
            }
            // Remove the old (right_key, old_left_val) from right_to_left_data_
            right_to_left_data_.erase(it_r_old);
        }

        // Insert the new pair
        auto it_l_new = std::lower_bound(left_to_right_data_.begin(), left_to_right_data_.end(), left_key, left_pair_compare_);
        left_to_right_data_.emplace(it_l_new, left_key, right_key);

        auto it_r_new = std::lower_bound(right_to_left_data_.begin(), right_to_left_data_.end(), right_key, right_pair_compare_);
        right_to_left_data_.emplace(it_r_new, right_key, left_key);
    }

    // Erases by left key. Returns true if element was found and erased, false otherwise.
    bool erase_left(const Left& key) {
        auto it_l = find_left_iter(key);
        if (it_l == left_to_right_data_.end()) {
            return false; // Key not found in left view
        }

        Right corresponding_right_key = it_l->second;
        left_to_right_data_.erase(it_l); // Erase from left-to-right map

        auto it_r = find_right_iter(corresponding_right_key);
        if (it_r != right_to_left_data_.end()) { // Should always be found if bimap is consistent
            right_to_left_data_.erase(it_r); // Erase from right-to-left map
        }
        return true;
    }

    // Erases by right key. Returns true if element was found and erased, false otherwise.
    bool erase_right(const Right& key) {
        auto it_r = find_right_iter(key);
        if (it_r == right_to_left_data_.end()) {
            return false; // Key not found in right view
        }

        Left corresponding_left_key = it_r->second;
        right_to_left_data_.erase(it_r); // Erase from right-to-left map

        auto it_l = find_left_iter(corresponding_left_key);
        if (it_l != left_to_right_data_.end()) { // Should always be found
            left_to_right_data_.erase(it_l); // Erase from left-to-right map
        }
        return true;
    }

    // --- Iterators ---
    // For VectorBiMap, iterators can directly use the underlying vector's iterators
    // as they already provide random access and point to the std::pair objects.

    // Left view iterators (iterate over std::pair<Left, Right>)
    using left_iterator = typename std::vector<left_value_type>::iterator;
    using const_left_iterator = typename std::vector<left_value_type>::const_iterator;

    left_iterator left_begin() { return left_to_right_data_.begin(); }
    left_iterator left_end() { return left_to_right_data_.end(); }
    const_left_iterator left_begin() const { return left_to_right_data_.cbegin(); }
    const_left_iterator left_end() const { return left_to_right_data_.cend(); }
    const_left_iterator left_cbegin() const { return left_to_right_data_.cbegin(); }
    const_left_iterator left_cend() const { return left_to_right_data_.cend(); }

    // Right view iterators (iterate over std::pair<Right, Left>)
    using right_iterator = typename std::vector<right_value_type>::iterator;
    using const_right_iterator = typename std::vector<right_value_type>::const_iterator;

    right_iterator right_begin() { return right_to_left_data_.begin(); }
    right_iterator right_end() { return right_to_left_data_.end(); }
    const_right_iterator right_begin() const { return right_to_left_data_.cbegin(); }
    const_right_iterator right_end() const { return right_to_left_data_.cend(); }
    const_right_iterator right_cbegin() const { return right_to_left_data_.cbegin(); }
    const_right_iterator right_cend() const { return right_to_left_data_.cend(); }

    // Default iterators (provide access to the left-to-right view by default)
    // These match std::map's iterator interface.
    left_iterator begin() { return left_begin(); }
    left_iterator end() { return left_end(); }
    const_left_iterator begin() const { return left_cbegin(); }
    const_left_iterator end() const { return left_cend(); }
    const_left_iterator cbegin() const { return left_cbegin(); }
    const_left_iterator cend() const { return left_cend(); }

    // Note on Views:
    // The existing hash-based BiMap in the repository has `left()` and `right()` methods
    // that return view objects, which then provide begin()/end().
    // For VectorBiMap, since the iterators are just vector iterators, providing
    // `left_begin/end` and `right_begin/end` directly is simpler and achieves the same goal
    // of allowing iteration over both perspectives of the bimap.
    // Creating separate view classes would add complexity without much benefit here,
    // as the iterators themselves are already full-featured.

}; // class VectorBiMap

// Non-member swap
template<typename L, typename R, typename CL, typename CR>
void swap(VectorBiMap<L, R, CL, CR>& lhs, VectorBiMap<L, R, CL, CR>& rhs) noexcept {
    lhs.swap(rhs);
}

} // namespace cpp_collections
