#pragma once

#include <set>
#include <map>
#include <vector>
#include <string> // Common type for T or G, good to include for users
#include <functional> // For std::less
#include <algorithm>  // For std::set_intersection, std::set_union
#include <iterator>   // For std::inserter

namespace cpp_collections {

template <
    typename T,
    typename G,
    typename ItemCompare = std::less<T>,
    typename GroupCompare = std::less<G>
>
class GroupedSet {
public:
    // Type aliases
    using item_type = T;
    using group_type = G;
    using item_set_type = std::set<T, ItemCompare>;
    using group_set_type = std::set<G, GroupCompare>;

private:
    // Internal data structures
    item_set_type all_items_;
    std::map<G, item_set_type, GroupCompare> group_to_items_;
    std::map<T, group_set_type, ItemCompare> item_to_groups_;

    // Comparators (store instances if they might have state, though std::less doesn't)
    ItemCompare item_comparator_;
    GroupCompare group_comparator_;

public:
    // Constructor
    GroupedSet(const ItemCompare& item_comp = ItemCompare(),
               const GroupCompare& group_comp = GroupCompare())
        : item_comparator_(item_comp), group_comparator_(group_comp) {}

    // Default copy/move constructors and assignment operators should be fine
    GroupedSet(const GroupedSet&) = default;
    GroupedSet(GroupedSet&&) = default;
    GroupedSet& operator=(const GroupedSet&) = default;
    GroupedSet& operator=(GroupedSet&&) = default;
    ~GroupedSet() = default;

    // --- Modification methods ---

    /**
     * @brief Adds an item to the global set of items.
     * The item will not belong to any group until explicitly added to one.
     * @param item The item to add.
     * @return True if the item was newly added, false if it already existed.
     */
    bool add_item(const T& item) {
        return all_items_.insert(item).second;
    }

    /**
     * @brief Adds an item to a specific group.
     * If the item does not exist in the GroupedSet, it's added to the global set of items.
     * If the group does not exist, it is created.
     * @param item The item to add to the group.
     * @param group The group to add the item to.
     * @return True if the item was newly added to this specific group,
     *         false if it was already in this group.
     */
    bool add_item_to_group(const T& item, const G& group) {
        all_items_.insert(item); // Ensure item exists globally

        // Add group to item's list of groups
        item_to_groups_[item].insert(group);

        // Add item to group's list of items
        // The return value reflects whether the item was new to this group.
        return group_to_items_[group].insert(item).second;
    }

    /**
     * @brief Removes an item from a specific group.
     * The item itself is not removed from the GroupedSet's global item list.
     * If the group becomes empty after removing the item, the group itself is not automatically deleted.
     * If the item no longer belongs to any group, its entry in item_to_groups_ is cleared but not removed.
     * @param item The item to remove from the group.
     * @param group The group from which to remove the item.
     * @return True if the item was found in the group and removed, false otherwise.
     */
    bool remove_item_from_group(const T& item, const G& group) {
        bool removed_from_group = false;
        auto group_it = group_to_items_.find(group);
        if (group_it != group_to_items_.end()) {
            if (group_it->second.erase(item) > 0) {
                removed_from_group = true;
            }
            // Design choice: Do not remove group if it becomes empty. User can call remove_group if desired.
        }

        auto item_it = item_to_groups_.find(item);
        if (item_it != item_to_groups_.end()) {
            item_it->second.erase(group);
            // Design choice: Do not remove item's entry from item_to_groups_ if its group set becomes empty.
            // It might still be in all_items_ and could be added to groups later.
        }
        return removed_from_group;
    }

    /**
     * @brief Removes an item completely from the GroupedSet.
     * This includes removing it from the global item list and from all groups it belonged to.
     * @param item The item to remove.
     * @return True if the item existed and was removed, false otherwise.
     */
    bool remove_item(const T& item) {
        if (all_items_.erase(item) == 0) {
            return false; // Item didn't exist in the global list
        }

        auto item_groups_it = item_to_groups_.find(item);
        if (item_groups_it != item_to_groups_.end()) {
            // Iterate over a copy of the groups, as we are modifying group_to_items_
            group_set_type groups_item_belonged_to = item_groups_it->second;
            for (const G& group_key : groups_item_belonged_to) {
                auto group_items_it = group_to_items_.find(group_key);
                if (group_items_it != group_to_items_.end()) {
                    group_items_it->second.erase(item);
                    // Optional: if (group_items_it->second.empty()) { group_to_items_.erase(group_items_it); }
                }
            }
            item_to_groups_.erase(item_groups_it); // Remove the item's entry from reverse map
        }
        return true;
    }

    /**
     * @brief Removes a group and dissociates all its items from it.
     * Items that were in this group are not removed from the GroupedSet's
     * global item list, nor are they removed from other groups they might belong to.
     * @param group The group to remove.
     * @return True if the group existed and was removed, false otherwise.
     */
    bool remove_group(const G& group) {
        auto group_items_it = group_to_items_.find(group);
        if (group_items_it == group_to_items_.end()) {
            return false; // Group didn't exist
        }

        // Iterate over a copy of items in the group to avoid iterator invalidation issues
        item_set_type items_in_group = group_items_it->second;
        for (const T& item_key : items_in_group) {
            auto item_groups_it = item_to_groups_.find(item_key);
            if (item_groups_it != item_to_groups_.end()) {
                item_groups_it->second.erase(group);
                // Optional: if (item_groups_it->second.empty()) { item_to_groups_.erase(item_groups_it); }
            }
        }
        group_to_items_.erase(group_items_it); // Remove the group itself
        return true;
    }

    /**
     * @brief Clears all items, groups, and memberships from the GroupedSet.
     */
    void clear() {
        all_items_.clear();
        group_to_items_.clear();
        item_to_groups_.clear();
    }

    // --- Query methods ---

    /**
     * @brief Checks if an item exists in the GroupedSet (regardless of group membership).
     * @param item The item to check.
     * @return True if the item exists, false otherwise.
     */
    bool item_exists(const T& item) const {
        return all_items_.count(item) > 0;
    }

    /**
     * @brief Checks if a group exists (i.e., has been created, possibly empty).
     * @param group The group to check.
     * @return True if the group exists, false otherwise.
     */
    bool group_exists(const G& group) const {
        return group_to_items_.count(group) > 0;
    }

    /**
     * @brief Checks if a specific item belongs to a specific group.
     * @param item The item to check.
     * @param group The group to check.
     * @return True if the item is in the group, false otherwise.
     */
    bool is_item_in_group(const T& item, const G& group) const {
        auto it = group_to_items_.find(group);
        if (it != group_to_items_.end()) {
            return it->second.count(item) > 0;
        }
        return false;
    }

    /**
     * @brief Retrieves a set of items belonging to a specific group.
     * @param group The group whose items are to be retrieved.
     * @return A copy of the set of items in the group. Returns an empty set if the group doesn't exist.
     */
    item_set_type get_items_in_group(const G& group) const {
        auto it = group_to_items_.find(group);
        if (it != group_to_items_.end()) {
            return it->second; // Returns a copy
        }
        return item_set_type(item_comparator_); // Return empty set with correct comparator
    }

    /**
     * @brief Retrieves a set of groups that a specific item belongs to.
     * @param item The item whose groups are to be retrieved.
     * @return A copy of the set of groups the item belongs to. Returns an empty set if the item doesn't exist or isn't in any group.
     */
    group_set_type get_groups_for_item(const T& item) const {
        auto it = item_to_groups_.find(item);
        if (it != item_to_groups_.end()) {
            return it->second; // Returns a copy
        }
        return group_set_type(group_comparator_); // Return empty set with correct comparator
    }

    /**
     * @brief Retrieves all unique items stored in the GroupedSet, regardless of group membership.
     * @return A const reference to the set of all items.
     */
    const item_set_type& get_all_items() const {
        return all_items_;
    }

    /**
     * @brief Retrieves a list of all unique group identifiers currently active.
     * @return A vector containing all group keys.
     */
    std::vector<G> get_all_groups() const {
        std::vector<G> group_keys;
        group_keys.reserve(group_to_items_.size());
        for (const auto& pair : group_to_items_) {
            group_keys.push_back(pair.first);
        }
        // Optionally sort if a specific order is desired, but map keys are already somewhat ordered.
        // For std::map, keys are already sorted by GroupCompare.
        return group_keys;
    }

    /**
     * @brief Retrieves items that are present in ALL of the specified groups.
     * @param groups A vector of group identifiers.
     * @return A set of items that are members of every group in the input vector.
     *         Returns an empty set if the groups vector is empty or no items match.
     */
    item_set_type get_items_in_all_groups(const std::vector<G>& groups) const {
        if (groups.empty()) {
            return item_set_type(item_comparator_);
        }

        // Start with items from the first group (or an empty set if group doesn't exist)
        item_set_type result_items = get_items_in_group(groups[0]);
        if (result_items.empty() && groups.size() > 1) { // If first group is empty, intersection will be empty
             return item_set_type(item_comparator_);
        }


        for (size_t i = 1; i < groups.size(); ++i) {
            item_set_type current_group_items = get_items_in_group(groups[i]);
            item_set_type intersection(item_comparator_);
            std::set_intersection(result_items.begin(), result_items.end(),
                                  current_group_items.begin(), current_group_items.end(),
                                  std::inserter(intersection, intersection.begin()),
                                  item_comparator_);
            result_items = std::move(intersection);
            if (result_items.empty()) { // Optimization: if intersection is empty, further work is futile
                break;
            }
        }
        return result_items;
    }

    /**
     * @brief Retrieves items that are present in ANY of the specified groups.
     * @param groups A vector of group identifiers.
     * @return A set of items that are members of at least one group in the input vector.
     */
    item_set_type get_items_in_any_group(const std::vector<G>& groups) const {
        item_set_type result_items(item_comparator_);
        for (const G& group_key : groups) {
            item_set_type current_group_items = get_items_in_group(group_key);
            result_items.insert(current_group_items.begin(), current_group_items.end());
        }
        return result_items;
    }

    /**
     * @brief Retrieves items that are in the GroupedSet but do not belong to any group.
     * @return A set of items that are not associated with any group.
     */
    item_set_type get_ungrouped_items() const {
        item_set_type ungrouped(item_comparator_);
        for (const T& item : all_items_) {
            auto it = item_to_groups_.find(item);
            // Item is ungrouped if it's not in item_to_groups_ or its set of groups is empty.
            if (it == item_to_groups_.end() || it->second.empty()) {
                ungrouped.insert(item);
            }
        }
        return ungrouped;
    }

    // --- Size/utility methods ---

    /**
     * @brief Returns the total number of unique items in the GroupedSet.
     * @return The number of unique items.
     */
    size_t size() const {
        return all_items_.size();
    }

    /**
     * @brief Checks if the GroupedSet contains any items.
     * @return True if there are no items, false otherwise.
     */
    bool empty() const {
        return all_items_.empty();
    }

    /**
     * @brief Returns the number of distinct groups that have at least one item or have been explicitly created.
     * @return The number of groups.
     */
    size_t group_count() const {
        return group_to_items_.size();
    }

    /**
     * @brief Returns the number of items in a specific group.
     * @param group The group identifier.
     * @return The number of items in the group, or 0 if the group doesn't exist.
     */
    size_t items_in_group_count(const G& group) const {
        auto it = group_to_items_.find(group);
        if (it != group_to_items_.end()) {
            return it->second.size();
        }
        return 0;
    }

    /**
     * @brief Returns the number of groups a specific item belongs to.
     * @param item The item identifier.
     * @return The number of groups the item is a member of, or 0 if the item doesn't exist or isn't in any groups.
     */
    size_t groups_for_item_count(const T& item) const {
        auto it = item_to_groups_.find(item);
        if (it != item_to_groups_.end()) {
            return it->second.size();
        }
        return 0;
    }

}; // class GroupedSet

} // namespace cpp_collections
