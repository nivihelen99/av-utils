#ifndef CORD_H
#define CORD_H

#include <string>
#include <memory> // For std::shared_ptr, std::make_shared
#include <variant>  // For node representation (leaf or internal)
#include <stdexcept> // For std::logic_error
#include <vector> // For collecting characters in ToString

// Forward declaration
#include <iostream> // For debugging, remove later
#include <vector>   // For std::vector in ToString
#include <numeric>  // For std::accumulate in length calculation (alternative)
#include <cstring>  // For strlen

// Forward declaration
class Cord;

/**
 * @brief Namespace for Cord implementation details.
 * Users should not directly interact with types in this namespace.
 */
namespace cord_detail {

struct LeafNode {
    std::string data;
    // Consider adding a small buffer for small string optimization (SSO) here
    // or using a string type that has SSO. For now, std::string is fine.

    explicit LeafNode(std::string s) : data(std::move(s)) {}
    explicit LeafNode(const char* s) : data(s ? s : "") {} // Handle null C-string
    explicit LeafNode(const char* s, size_t len) : data(s, len) {}
};

/**
 * @brief Represents an internal node in the Cord's rope-like structure.
 * An InternalNode concatenates two child Cord objects.
 */
struct InternalNode; // Forward declaration for mutual dependency

// NodeVariant stores either a LeafNode or a shared_ptr to an InternalNode.
// Using shared_ptr for InternalNode allows multiple Cord instances to share
// the same internal structure if they result from operations like substring or concatenation.
using NodeVariant = std::variant<LeafNode, std::shared_ptr<InternalNode>>;

struct InternalNode {
    // Using std::shared_ptr<const Cord> for children to emphasize that
    // Cords are immutable values and their internal structure can be shared.
    // This also helps break potential ownership cycles if Cord held shared_ptr<InternalNode>
    // and InternalNode held shared_ptr<Cord> directly.
    std::shared_ptr<const Cord> left;  ///< Pointer to the left child Cord.
    std::shared_ptr<const Cord> right; ///< Pointer to the right child Cord.
    size_t length_left; ///< Cached length of the left child, for efficient indexing.

    /**
     * @brief Constructs an InternalNode.
     * @param l Left child Cord.
     * @param r Right child Cord.
     */
    InternalNode(std::shared_ptr<const Cord> l, std::shared_ptr<const Cord> r);
};

} // namespace cord_detail

/**
 * @class Cord
 * @brief A rope-like data structure for efficient manipulation of large strings.
 *
 * Cords represent strings as a tree of smaller string fragments. This allows for
 * operations like concatenation and substring to be performed more efficiently
 * than with traditional flat strings, especially for large amounts of text,
 * by minimizing data copying and allowing shared substructures.
 */
class Cord {
public:
    // --- Constructors ---

    /**
     * @brief Default constructor. Creates an empty Cord.
     * Time complexity: O(1).
     */
    Cord() : node_(std::make_shared<cord_detail::NodeVariant>(cord_detail::LeafNode(""))), total_length_(0) {}

    /**
     * @brief Constructs a Cord from a C-style string.
     * @param s Null-terminated C-style string. If nullptr, an empty Cord is created.
     * Time complexity: O(L) where L is the length of s, due to string copying.
     */
    Cord(const char* s) : total_length_(0) {
        if (s == nullptr || *s == '\0') {
            node_ = std::make_shared<cord_detail::NodeVariant>(cord_detail::LeafNode(""));
        } else {
            init_from_string(std::string(s));
        }
    }

    /**
     * @brief Constructs a Cord from an std::string (by copying).
     * @param s The string to copy.
     * Time complexity: O(L) where L is the length of s.
     */
    Cord(const std::string& s) : total_length_(0) {
        init_from_string(s);
    }

    /**
     * @brief Constructs a Cord from an std::string (by moving).
     * @param s The string to move from. s is left in a valid but unspecified state.
     * Time complexity: O(1) if std::string move is O(1), otherwise O(L).
     */
    Cord(std::string&& s) : total_length_(0) { // Move constructor from std::string
        init_from_string(std::move(s));
    }

    /**
     * @brief Copy constructor.
     * Creates a new Cord that shares the underlying tree structure with `other`.
     * This is a shallow copy of the Cord object itself, but a deep copy of the
     * shared_ptr to the root node, leading to shared ownership of the data.
     * Time complexity: O(1).
     */
    Cord(const Cord& other) = default;

    /**
     * @brief Move constructor.
     * Transfers ownership of the underlying data from `other` to this Cord.
     * `other` is left in a valid empty state.
     * Time complexity: O(1).
     */
    Cord(Cord&& other) noexcept
        : node_(std::move(other.node_)), total_length_(other.total_length_) {
        // Leave other in a valid empty state
        other.node_ = std::make_shared<cord_detail::NodeVariant>(cord_detail::LeafNode(""));
        other.total_length_ = 0;
    }

    // --- Assignment Operators ---

    /**
     * @brief Copy assignment operator.
     * Shares the underlying tree structure with `other`.
     * Time complexity: O(1) (due to shared_ptr assignment).
     */
    Cord& operator=(const Cord& other) = default;

    /**
     * @brief Move assignment operator.
     * Transfers ownership from `other`. `other` is left in a valid empty state.
     * Time complexity: O(1).
     */
    Cord& operator=(Cord&& other) noexcept {
        if (this != &other) {
            node_ = std::move(other.node_);
            total_length_ = other.total_length_;
            // Leave other in a valid empty state
            other.node_ = std::make_shared<cord_detail::NodeVariant>(cord_detail::LeafNode(""));
            other.total_length_ = 0;
        }
        return *this;
    }

    /**
     * @brief Assigns from an std::string (by copying).
     * Time complexity: O(L) where L is the length of s.
     */
    Cord& operator=(const std::string& s) {
        init_from_string(s);
        return *this;
    }

    /**
     * @brief Assigns from an std::string (by moving).
     * Time complexity: O(1) if std::string move is O(1), otherwise O(L).
     */
    Cord& operator=(std::string&& s) {
        init_from_string(std::move(s));
        return *this;
    }

    /**
     * @brief Assigns from a C-style string.
     * Time complexity: O(L) where L is the length of s.
     */
    Cord& operator=(const char* s) {
        if (s == nullptr) {
            init_from_string(std::string(""));
        } else {
            init_from_string(std::string(s));
        }
        return *this;
    }

    // --- Basic Operations ---

    /**
     * @brief Returns the total length of the string represented by the Cord.
     * Time complexity: O(1).
     */
    size_t length() const {
        return total_length_;
    }

    /**
     * @brief Checks if the Cord is empty (length is 0).
     * Time complexity: O(1).
     */
    bool empty() const {
        return total_length_ == 0;
    }

    /**
     * @brief Clears the Cord, making it represent an empty string.
     * Time complexity: O(1).
     */
    void clear() {
        // Reset to an empty Cord state
        node_ = std::make_shared<cord_detail::NodeVariant>(cord_detail::LeafNode(""));
        total_length_ = 0;
    }

    // --- Concatenation ---

    /**
     * @brief Concatenates this Cord with another Cord.
     * @param other The Cord to append.
     * @return A new Cord representing the concatenation.
     * Time complexity: O(1) to create a new internal node (tree depth may increase).
     */
    Cord operator+(const Cord& other) const {
        if (this->empty()) return other;
        if (other.empty()) return *this;
        // Create new Cords from shared_ptr to const this/other for InternalNode
        return Cord(std::make_shared<const Cord>(*this), std::make_shared<const Cord>(other));
    }

    /**
     * @brief Concatenates this Cord with an std::string.
     * @param s_other The string to append.
     * @return A new Cord representing the concatenation.
     */
    Cord operator+(const std::string& s_other) const {
        return *this + Cord(s_other);
    }

    /**
     * @brief Concatenates this Cord with a C-style string.
     * @param c_other The C-style string to append.
     * @return A new Cord representing the concatenation.
     */
    Cord operator+(const char* c_other) const {
        return *this + Cord(c_other);
    }

    /**
     * @brief Friend function for `std::string + Cord` concatenation.
     */
    friend Cord operator+(const std::string& s_lhs, const Cord& rhs) {
        return Cord(s_lhs) + rhs;
    }
    /**
     * @brief Friend function for `const char* + Cord` concatenation.
     */
    friend Cord operator+(const char* c_lhs, const Cord& rhs) {
        return Cord(c_lhs) + rhs;
    }

    // --- Character Access ---
    /**
     * @brief Accesses the character at the specified index with bounds checking.
     * @param index The 0-based index of the character.
     * @return The character at the specified index.
     * @throws std::out_of_range if index is out of bounds.
     * Time complexity: O(depth of tree), which is O(log N) for balanced trees,
     * O(N) for degenerate trees.
     */
    char at(size_t index) const {
        if (index >= total_length_) {
            throw std::out_of_range("Cord::at() index out of bounds");
        }
        return char_at_recursive(index);
    }

    /**
     * @brief Accesses the character at the specified index.
     * No bounds checking is performed (like `std::string::operator[]`).
     * Behavior is undefined if index is out of bounds.
     * @param index The 0-based index of the character.
     * @return The character at the specified index.
     * Time complexity: O(depth of tree).
     */
    char operator[](size_t index) const {
        // As per std::string, operator[] has undefined behavior if index >= size().
        // For safety in debug, an assert could be added, or make it call at().
        // For performance, often unchecked. Let's make it call at() for now.
        return at(index);
    }

    // --- Substring ---
    /**
     * @brief Extracts a substring.
     * @param pos Starting position of the substring (0-based).
     * @param count Length of the substring. If `std::string::npos` or if `pos + count`
     *              exceeds cord length, extracts till the end.
     * @return A new Cord representing the substring.
     * @throws std::out_of_range if `pos` is greater than the Cord's length.
     * Time complexity: O(depth of tree + log K) where K is count, for creating new nodes.
     * Does not copy character data until ToString() is called on the substring.
     */
    Cord substr(size_t pos = 0, size_t count = std::string::npos) const {
        if (pos > total_length_) {
            throw std::out_of_range("Cord::substr() pos out of bounds");
        }
        size_t effective_count = count;
        if (count == std::string::npos || pos + count > total_length_) {
            effective_count = total_length_ - pos;
        }
        if (effective_count == 0) {
            return Cord(); // Return empty Cord
        }
        return substr_recursive(pos, effective_count);
    }

    // --- Conversion ---
    /**
     * @brief Converts the Cord to a standard std::string.
     * This operation involves concatenating all fragments and can be expensive
     * for very large Cords.
     * @return A std::string representation of the Cord.
     * Time complexity: O(N) where N is the total length of the Cord.
     */
    std::string ToString() const {
        std::string result;
        result.reserve(total_length_);
        to_string_recursive(result);
        return result;
    }

    // --- Destructor ---
    /**
     * @brief Destructor. Manages resource cleanup via `std::shared_ptr`.
     */
    ~Cord() = default; // Default is fine due to shared_ptr managing nodes

private:
    friend struct cord_detail::InternalNode; // Allow InternalNode to access Cord's private members
    friend class CordTest_Concatenation_Test;
    friend class CordTest_SubstrBasic_Test;
    friend struct cord_detail::InternalNode; // Allow InternalNode to access Cord's private members
    friend class CordTest_Concatenation_Test;
    friend class CordTest_SubstrBasic_Test;
    friend class CordTest_SubstrEdgeCases_Test;
    friend class CordTest_AtOperator_Test;

    // Data members
    std::shared_ptr<cord_detail::NodeVariant> node_;
    size_t total_length_;

    // Internal constructor for creating a Cord from a single leaf node string
    explicit Cord(cord_detail::LeafNode leaf)
        : node_(std::make_shared<cord_detail::NodeVariant>(std::move(leaf))),
          total_length_(std::get<cord_detail::LeafNode>(*node_).data.length()) {}

    // Internal constructor for creating a Cord from an internal node.
    // The shared_ptr to InternalNode is directly assigned.
    explicit Cord(std::shared_ptr<cord_detail::InternalNode> internal_node_ptr)
        : node_(std::make_shared<cord_detail::NodeVariant>(std::move(internal_node_ptr))),
          total_length_(0) {
        if (const auto* in_node_ptr_variant = std::get_if<std::shared_ptr<cord_detail::InternalNode>>(&*node_)) {
            if (const auto& in_node = *in_node_ptr_variant) { // Check if the shared_ptr itself is not null
                 total_length_ = (in_node->left ? in_node->left->length() : 0) +
                                (in_node->right ? in_node->right->length() : 0);
            } else {
                 throw std::logic_error("Cord created with null InternalNode shared_ptr");
            }
        } else {
            // This should not happen if constructor logic is correct
            throw std::logic_error("Cord internal constructor variant mismatch");
        }
    }


    // Helper to initialize from a string
    void init_from_string(const std::string& s) {
        if (s.empty()) {
            node_ = std::make_shared<cord_detail::NodeVariant>(cord_detail::LeafNode(""));
            total_length_ = 0;
        } else {
            node_ = std::make_shared<cord_detail::NodeVariant>(cord_detail::LeafNode(s));
            total_length_ = s.length();
        }
    }
    void init_from_string(std::string&& s) {
         if (s.empty()) {
            node_ = std::make_shared<cord_detail::NodeVariant>(cord_detail::LeafNode(""));
            total_length_ = 0;
        } else {
            node_ = std::make_shared<cord_detail::NodeVariant>(cord_detail::LeafNode(std::move(s)));
            total_length_ = std::get<cord_detail::LeafNode>(*node_).data.length(); // Get length after move
        }
    }

    // Private constructor for concatenation, taking shared_ptr<const Cord>
    Cord(std::shared_ptr<const Cord> left_child, std::shared_ptr<const Cord> right_child)
        : total_length_((left_child ? left_child->length() : 0) + (right_child ? right_child->length() : 0)) {
        if (!left_child || !right_child) {
            throw std::logic_error("Children for internal Cord node cannot be null");
        }
        // Important: InternalNode constructor now takes shared_ptr<const Cord>
        auto internal_node_ptr = std::make_shared<cord_detail::InternalNode>(std::move(left_child), std::move(right_child));
        node_ = std::make_shared<cord_detail::NodeVariant>(std::move(internal_node_ptr));
    }

    // Recursive helper for ToString
    void to_string_recursive(std::string& out_str) const {
        if (!node_) return; // Should not happen with current design where node_ is always initialized

        std::visit([&out_str](auto&& arg) {
            using T_variant = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T_variant, cord_detail::LeafNode>) {
                out_str.append(arg.data);
            } else if constexpr (std::is_same_v<T_variant, std::shared_ptr<cord_detail::InternalNode>>) {
                if (arg) { // Check if the shared_ptr to InternalNode is not null
                    if (arg->left) arg->left->to_string_recursive(out_str);
                    if (arg->right) arg->right->to_string_recursive(out_str);
                }
            }
        }, *node_);
    }

    // Recursive helper for character access
    char char_at_recursive(size_t index) const {
        if (!node_) throw std::logic_error("Accessing char_at_recursive on uninitialized Cord node");

        return std::visit([index, this](auto&& arg) -> char {
            using T_variant = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T_variant, cord_detail::LeafNode>) {
                if (index < arg.data.length()) {
                    return arg.data[index];
                } else {
                    // This should be caught by the initial check in at()
                    throw std::out_of_range("Cord::char_at_recursive index out of bounds in LeafNode");
                }
            } else if constexpr (std::is_same_v<T_variant, std::shared_ptr<cord_detail::InternalNode>>) {
                if (!arg) throw std::logic_error("Null InternalNode encountered in char_at_recursive");
                if (!arg->left) throw std::logic_error("Null left child in InternalNode");

                size_t left_len = arg->length_left; // Use stored length
                if (index < left_len) {
                    return arg->left->char_at_recursive(index);
                } else {
                    if (!arg->right) throw std::logic_error("Null right child in InternalNode");
                    return arg->right->char_at_recursive(index - left_len);
                }
            }
            // Should not be reached if variant is exhaustive and nodes are valid
            throw std::logic_error("Unhandled variant type in char_at_recursive");
        }, *node_);
    }

    // Recursive helper for substring
    Cord substr_recursive(size_t pos, size_t count) const {
        if (count == 0) return Cord(); // Empty substring
        if (!node_) throw std::logic_error("substr_recursive on uninitialized Cord node");

        return std::visit([pos, count, this](auto&& arg) -> Cord {
            using T_variant = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T_variant, cord_detail::LeafNode>) {
                // Substring from a leaf node
                if (pos >= arg.data.length()) return Cord(); // pos is out of bounds for this leaf
                size_t len_to_take = std::min(count, arg.data.length() - pos);
                return Cord(arg.data.substr(pos, len_to_take));
            } else if constexpr (std::is_same_v<T_variant, std::shared_ptr<cord_detail::InternalNode>>) {
                if (!arg) throw std::logic_error("Null InternalNode in substr_recursive");
                if (!arg->left || !arg->right) throw std::logic_error("Null child in InternalNode for substr");

                size_t left_len = arg->length_left;

                Cord res_left, res_right;

                if (pos < left_len) { // Substring starts in the left child
                    size_t count_from_left = std::min(count, left_len - pos);
                    res_left = arg->left->substr_recursive(pos, count_from_left);

                    if (count > count_from_left) { // Substring spans into the right child
                        size_t count_from_right = count - count_from_left;
                        res_right = arg->right->substr_recursive(0, count_from_right);
                    }
                } else { // Substring starts entirely in the right child
                    res_right = arg->right->substr_recursive(pos - left_len, count);
                }

                if (res_left.empty()) return res_right;
                if (res_right.empty()) return res_left;
                return res_left + res_right; // Concatenate results from children
            }
            // Should not be reached
            throw std::logic_error("Unhandled variant type in substr_recursive");
        }, *node_);
    }
};


// Implementation of cord_detail::InternalNode constructor needs Cord definition
namespace cord_detail {
// Constructor now takes shared_ptr<const Cord>
inline InternalNode::InternalNode(std::shared_ptr<const Cord> l, std::shared_ptr<const Cord> r)
    : left(std::move(l)), right(std::move(r)) {
    if (!left) { // Right can be empty for a single-sided internal node if we allowed that (not current design)
        throw std::logic_error("InternalNode left child cannot be null");
    }
    length_left = left->length(); // Store length of left child
    // Depth calculation can be added if balancing is implemented
    // depth = 1 + std::max(left_ ? left_->depth_ : 0, right_ ? right_->depth_ : 0);
}
} // namespace cord_detail


#endif // CORD_H
