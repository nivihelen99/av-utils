#pragma once

#include <vector>
#include <set>
#include <unordered_map>
#include <string> // For ItemIDType example, can be anything hashable
#include <functional> // For std::hash, std::equal_to, std::reference_wrapper
#include <optional>   // For std::optional
#include <stdexcept>  // For std::invalid_argument
#include <algorithm>  // For std::min, std::max
#include <limits>     // For std::numeric_limits
#include <memory>     // For std::allocator, std::allocator_traits

namespace cpp_collections {

// Forward declaration
template<
    typename ItemIDType,
    typename ValueType = double,
    typename CostType = double,
    typename Hash = std::hash<ItemIDType>,
    typename KeyEqual = std::equal_to<ItemIDType>,
    typename Allocator = std::allocator<char> // Placeholder, will be rebound
>
class TopNByRatioSelector;

// Internal structure to hold item details
template<typename ItemIDType, typename ValueType, typename CostType>
struct ItemEntry {
    ItemIDType id;
    ValueType value;
    CostType cost;
    double ratio; // value / cost

    // Equality operator for find/erase in set if ItemEntry itself is stored directly
    // and for testing. Comparison based on ID should be sufficient.
    bool operator==(const ItemEntry& other) const {
        return id == other.id;
    }
    // Inequality for convenience
    bool operator!=(const ItemEntry& other) const {
        return !(*this == other);
    }
};

// Comparator for sorting ItemEntry objects in the std::set
// Sorts by ratio (descending), then by ID (ascending for tie-breaking and uniqueness)
template<typename ItemIDType, typename ValueType, typename CostType>
struct RatioCompare {
    using Entry = ItemEntry<ItemIDType, ValueType, CostType>;
    bool operator()(const Entry& a, const Entry& b) const {
        if (a.ratio != b.ratio) {
            return a.ratio > b.ratio; // Higher ratio comes first
        }
        // Tie-breaking by ID (ascending) to ensure unique ordering
        // and deterministic behavior. std::set requires a strict weak ordering.
        return a.id < b.id;
    }
};


template<
    typename ItemIDType,
    typename ValueType,
    typename CostType,
    typename Hash,
    typename KeyEqual,
    typename Allocator
>
class TopNByRatioSelector {
public:
    // Type aliases
    using item_id_type = ItemIDType;
    using value_type = ValueType;
    using cost_type = CostType;
    using item_entry_type = ItemEntry<ItemIDType, ValueType, CostType>;

private:
    // Allocator rebinding for internal containers
    template<typename T>
    using ReboundAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<T>;

    using UMapAlloc = ReboundAllocator<std::pair<const ItemIDType, item_entry_type>>;
    using SetAlloc = ReboundAllocator<item_entry_type>;

    using ItemMap = std::unordered_map<ItemIDType, item_entry_type, Hash, KeyEqual, UMapAlloc>;
    using OrderedSet = std::set<item_entry_type, RatioCompare<ItemIDType, ValueType, CostType>, SetAlloc>;

    ItemMap item_data_map_;
    OrderedSet sorted_items_by_ratio_;

    Hash hasher_;
    KeyEqual key_equal_;
    Allocator allocator_; // Store a copy of the allocator

public:
    // --- Constructors ---
    explicit TopNByRatioSelector(const Hash& hash = Hash(),
                                 const KeyEqual& equal = KeyEqual(),
                                 const Allocator& alloc = Allocator())
        : item_data_map_(0, hash, equal, UMapAlloc(alloc)),
          sorted_items_by_ratio_(RatioCompare<ItemIDType, ValueType, CostType>(), SetAlloc(alloc)),
          hasher_(hash),
          key_equal_(equal),
          allocator_(alloc) {}

    explicit TopNByRatioSelector(const Allocator& alloc)
        : item_data_map_(0, Hash(), KeyEqual(), UMapAlloc(alloc)),
          sorted_items_by_ratio_(RatioCompare<ItemIDType, ValueType, CostType>(), SetAlloc(alloc)),
          hasher_(Hash()),
          key_equal_(KeyEqual()),
          allocator_(alloc) {}

    // Copy constructor
    TopNByRatioSelector(const TopNByRatioSelector& other)
        : item_data_map_(other.item_data_map_, UMapAlloc(other.allocator_)),
          sorted_items_by_ratio_(other.sorted_items_by_ratio_, SetAlloc(other.allocator_)),
          hasher_(other.hasher_),
          key_equal_(other.key_equal_),
          allocator_(other.allocator_) {}

    // Move constructor
    TopNByRatioSelector(TopNByRatioSelector&& other) noexcept
        : item_data_map_(std::move(other.item_data_map_)),
          sorted_items_by_ratio_(std::move(other.sorted_items_by_ratio_)),
          hasher_(std::move(other.hasher_)),
          key_equal_(std::move(other.key_equal_)),
          allocator_(std::move(other.allocator_)) {}

    // Copy assignment
    TopNByRatioSelector& operator=(const TopNByRatioSelector& other) {
        if (this == &other) {
            return *this;
        }
        // Ensure allocator propagation on copy assignment if POCMA is true
        if (std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value) {
            allocator_ = other.allocator_;
        }
        // Create new containers with potentially new allocator
        item_data_map_ = ItemMap(other.item_data_map_, UMapAlloc(allocator_));
        sorted_items_by_ratio_ = OrderedSet(other.sorted_items_by_ratio_, SetAlloc(allocator_));

        hasher_ = other.hasher_;
        key_equal_ = other.key_equal_;
        return *this;
    }

    // Move assignment
    TopNByRatioSelector& operator=(TopNByRatioSelector&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        // Ensure allocator propagation on move assignment if POCMA is true
        // or if allocators are equal. If POCMA is false and allocators differ,
        // standard says behavior is to move elements one by one.
        // For simplicity here, we'll assume if POCMA is false, we might need element-wise move.
        // However, unordered_map and set move assignment handles this.

        // Clear current state first if allocators might not propagate or differ
        // and we want to ensure a clean move. For now, directly move.
        // This might need more nuanced handling based on allocator_traits::propagate_on_container_move_assignment.

        item_data_map_ = std::move(other.item_data_map_);
        sorted_items_by_ratio_ = std::move(other.sorted_items_by_ratio_);
        hasher_ = std::move(other.hasher_);
        key_equal_ = std::move(other.key_equal_);

        if (std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value) {
            allocator_ = std::move(other.allocator_);
        } else {
            // If allocator doesn't propagate on move and is not equal,
            // the move operations on map/set might have specific behaviors.
            // For simplicity, if it doesn't propagate, our allocator_ member remains.
            // This implies that if allocators are different and POCMA is false,
            // the elements themselves must be movable with the old allocator.
            // This is a complex area, keeping it simpler for now.
            // If allocators are different and POCMA is false, map/set move assignment
            // will perform element-wise move if their allocators are not equal.
            // If allocators are equal, then it's a fast pointer swap.
        }
        return *this;
    }

    // Destructor
    ~TopNByRatioSelector() = default;

    // --- Modifiers ---

    /**
     * @brief Adds a new item or updates an existing one.
     * Costs must be strictly positive. If cost <= 0, an std::invalid_argument is thrown.
     * @param id The ID of the item.
     * @param value The value of the item.
     * @param cost The cost of the item (must be > 0).
     * @return True if the item was successfully added or updated.
     * @throw std::invalid_argument if cost is not positive.
     */
    bool add_or_update_item(const ItemIDType& id, const ValueType& value, const CostType& cost) {
        if (cost <= static_cast<CostType>(0)) {
            throw std::invalid_argument("Item cost must be positive.");
        }

        double item_ratio = static_cast<double>(value) / static_cast<double>(cost);
        item_entry_type current_entry = {id, value, cost, item_ratio};

        auto map_it = item_data_map_.find(id);
        if (map_it != item_data_map_.end()) {
            // Item exists. Remove old entry from sorted_items_by_ratio_ first.
            // The item_entry_type in map_it->second contains the old state (specifically old ratio)
            // which is needed for correct removal from the set.
            if (sorted_items_by_ratio_.erase(map_it->second) == 0) {
                // This would indicate an inconsistency if the item was in map but not set.
                // For robustness, one might log an error or throw here,
                // but typically this shouldn't happen if logic is correct.
            }
            map_it->second = current_entry; // Update the entry in the map
        } else {
            // New item, just emplace it in the map.
            item_data_map_.emplace(id, current_entry);
        }

        // Insert the new or updated entry into the sorted set.
        // std::set::insert will handle cases where the "equivalent" element (based on RatioCompare)
        // might already exist if we were trying to insert an exact duplicate after an update
        // that didn't change ratio or ID, but RatioCompare includes ID for uniqueness.
        sorted_items_by_ratio_.insert(current_entry);
        return true;
    }

    /**
     * @brief Removes an item from the selector.
     * @param id The ID of the item to remove.
     * @return True if the item was found and removed, false otherwise.
     */
    bool remove_item(const ItemIDType& id) {
        auto map_it = item_data_map_.find(id);
        if (map_it == item_data_map_.end()) {
            return false; // Item not found
        }

        // Erase from set using the details (specifically the ratio and id for comparison)
        // from the entry stored in the map.
        if (sorted_items_by_ratio_.erase(map_it->second) == 0) {
            // Similar to add_or_update_item, indicates inconsistency if map has it but set doesn't.
        }

        // Erase from map
        item_data_map_.erase(map_it);

        return true;
    }

    /**
     * @brief Clears all items from the selector.
     */
    void clear() noexcept {
        item_data_map_.clear();
        sorted_items_by_ratio_.clear();
    }

    // --- Observers ---

    /**
     * @brief Checks if an item with the given ID exists.
     * @param id The ID of the item.
     * @return True if the item exists, false otherwise.
     */
    bool contains_item(const ItemIDType& id) const {
        return item_data_map_.count(id) > 0; // or item_data_map_.find(id) != item_data_map_.end();
    }

    /**
     * @brief Retrieves the details of an item.
     * @param id The ID of the item.
     * @return An std::optional containing the item_entry_type if found, otherwise std::nullopt.
     */
    std::optional<item_entry_type> get_item_details(const ItemIDType& id) const {
        auto it = item_data_map_.find(id);
        if (it == item_data_map_.end()) {
            return std::nullopt;
        }
        return it->second; // Returns a copy of the ItemEntry
    }

    // Placeholder for allocator_type, common practice
    using allocator_type = Allocator;

    size_t size() const noexcept {
        return item_data_map_.size();
    }

    bool empty() const noexcept {
        return item_data_map_.empty();
    }

    allocator_type get_allocator() const noexcept {
        return allocator_;
    }

    // --- Selector Methods ---

    /**
     * @brief Selects the top N items based on the highest value/cost ratio.
     * @param n The maximum number of items to select.
     * @return A vector of item_entry_type for the selected items, ordered by ratio.
     */
    std::vector<item_entry_type> select_top_n(size_t n) const {
        std::vector<item_entry_type> result;
        if (n == 0) {
            return result;
        }
        result.reserve(std::min(n, sorted_items_by_ratio_.size()));
        for (const auto& entry : sorted_items_by_ratio_) {
            if (result.size() >= n) {
                break;
            }
            result.push_back(entry);
        }
        return result;
    }

    /**
     * @brief Selects items with the best ratios whose cumulative cost does not exceed max_total_cost.
     * @param max_total_cost The maximum allowable cumulative cost.
     * @return A vector of item_entry_type for the selected items, ordered by ratio.
     */
    std::vector<item_entry_type> select_by_budget(const CostType& max_total_cost) const {
        std::vector<item_entry_type> result;
        CostType current_total_cost = static_cast<CostType>(0);

        if (max_total_cost < static_cast<CostType>(0)) { // Or just treat as 0
             return result; // No budget or negative budget
        }

        for (const auto& entry : sorted_items_by_ratio_) {
            if (current_total_cost + entry.cost <= max_total_cost) {
                result.push_back(entry);
                current_total_cost += entry.cost;
            }
        }
        return result;
    }

    /**
     * @brief Selects up to N items with the best ratios whose cumulative cost does not exceed max_total_cost.
     * @param n The maximum number of items to select.
     * @param max_total_cost The maximum allowable cumulative cost.
     * @return A vector of item_entry_type for the selected items, ordered by ratio.
     */
    std::vector<item_entry_type> select_top_n_by_budget(size_t n, const CostType& max_total_cost) const {
        std::vector<item_entry_type> result;
        CostType current_total_cost = static_cast<CostType>(0);

        if (n == 0 || max_total_cost < static_cast<CostType>(0)) {
            return result;
        }

        result.reserve(std::min(n, sorted_items_by_ratio_.size()));

        for (const auto& entry : sorted_items_by_ratio_) {
            if (result.size() >= n) {
                break;
            }
            if (current_total_cost + entry.cost <= max_total_cost) {
                result.push_back(entry);
                current_total_cost += entry.cost;
            }
        }
        return result;
    }
};

} // namespace cpp_collections
