#include "../include/tagged_ptr.h"
#include <iostream>
#include <vector> // For managing node memory in this example
#include <cstdlib> // For exit()

// Minimal assert for example, real tests are in test file.
#define ASSERT_TRUE(condition, message) \
    if (!(condition)) { \
        std::cerr << "Assertion failed in example: (" #condition ") is not true - " << (message) << std::endl; \
        exit(1); \
    }

// Define Color enum for the Red-Black Tree node
enum class Color : uint8_t {
    RED = 0,
    BLACK = 1
    // In a real RB tree, we might only need 1 bit, so BLACK could be 0, RED could be 1.
    // Or vice-versa. Let's use 1 bit.
};

// Simplified Node structure for demonstration
// Ensure Node is aligned enough for 1 tag bit (2^1 = 2 bytes)
// Most non-empty structs will be aligned to at least 4 or 8 bytes by default.
struct alignas(2) RBNode {
    int key;
    // TaggedPtr will store RBNode* and Color (1 bit)
    TaggedPtr<RBNode, 1> left;
    TaggedPtr<RBNode, 1> right;
    // Parent pointer could also be a TaggedPtr if it needed a tag
    RBNode* parent;

    RBNode(int k) : key(k), parent(nullptr) {
        // Children are initially null, color doesn't matter for null / can be default
        left.set(nullptr, static_cast<uint8_t>(Color::BLACK));
        right.set(nullptr, static_cast<uint8_t>(Color::BLACK));
    }

    void set_left_child(RBNode* node, Color color) {
        left.set(node, static_cast<uint8_t>(color));
        if (node) node->parent = this;
    }

    void set_right_child(RBNode* node, Color color) {
        right.set(node, static_cast<uint8_t>(color));
        if (node) node->parent = this;
    }

    RBNode* get_left_child() const {
        return left.get_ptr();
    }

    Color get_left_color() const {
        return static_cast<Color>(left.get_tag());
    }

    RBNode* get_right_child() const {
        return right.get_ptr();
    }

    Color get_right_color() const {
        return static_cast<Color>(right.get_tag());
    }

    // Helper to print color
    static const char* color_to_string(Color c) {
        return c == Color::RED ? "RED" : "BLACK";
    }
};

// A very simple "tree" manager for demonstration purposes
class SimpleTree {
public:
    RBNode* root_ = nullptr;
    std::vector<RBNode*> all_nodes_; // To manage memory easily for the example

    ~SimpleTree() {
        for (RBNode* node : all_nodes_) {
            delete node;
        }
    }

    RBNode* add_node(int key) {
        RBNode* new_node = new RBNode(key);
        all_nodes_.push_back(new_node);
        if (!root_) {
            root_ = new_node;
        }
        // This is not a real RB tree insertion, just adding nodes for demo
        return new_node;
    }

    void print_node_info(const RBNode* node) {
        if (!node) {
            std::cout << "Node: null" << std::endl;
            return;
        }
        std::cout << "Node Key: " << node->key << std::endl;

        RBNode* left_child = node->get_left_child();
        Color left_color = node->get_left_color();
        std::cout << "  Left Child: " << (left_child ? std::to_string(left_child->key) : "null");
        std::cout << ", Color: " << RBNode::color_to_string(left_color) << std::endl;

        RBNode* right_child = node->get_right_child();
        Color right_color = node->get_right_color();
        std::cout << "  Right Child: " << (right_child ? std::to_string(right_child->key) : "null");
        std::cout << ", Color: " << RBNode::color_to_string(right_color) << std::endl;
    }
};


int main() {
    std::cout << "TaggedPtr Example: Simplified Red-Black Tree Node" << std::endl;
    std::cout << "Max tag value for 1 bit: " << TaggedPtr<RBNode, 1>::max_tag() << std::endl;
    // Test alignment requirement (compile time)
    // TaggedPtr<RBNode, 3> tp_compile_fail; // RBNode alignof is likely >=2 but <8. This should fail.

    SimpleTree tree;

    RBNode* n10 = tree.add_node(10); // Root
    RBNode* n5  = tree.add_node(5);
    RBNode* n15 = tree.add_node(15);
    RBNode* n3  = tree.add_node(3);
    RBNode* n7  = tree.add_node(7);

    // Manually construct a small tree structure using TaggedPtr for color
    // Root is typically BLACK in an RB tree (after balancing)
    // Let's imagine n10 is root and black.
    // Children of a RED node must be BLACK.
    // For this demo, we'll just set some arbitrary colors.

    if (tree.root_ == n10) { // n10 is root
        // Let's say n5 is its RED left child
        n10->set_left_child(n5, Color::RED);
        // And n15 is its BLACK right child
        n10->set_right_child(n15, Color::BLACK);
    }

    // n5 (RED) must have BLACK children
    n5->set_left_child(n3, Color::BLACK);
    n5->set_right_child(n7, Color::BLACK);

    // n15 (BLACK) can have RED or BLACK children
    // Let's leave its children as null (implicitly BLACK from constructor)

    std::cout << "\n--- Tree Structure ---" << std::endl;
    tree.print_node_info(n10);
    tree.print_node_info(n5);
    tree.print_node_info(n15);
    tree.print_node_info(n3);
    tree.print_node_info(n7);

    std::cout << "\n--- Modifying a tag ---" << std::endl;
    std::cout << "n10's left child (" << n5->key << ") current color: "
              << RBNode::color_to_string(n10->get_left_color()) << std::endl;

    n10->left.set_tag(static_cast<uint8_t>(Color::BLACK)); // Change n5's color via n10's TaggedPtr

    std::cout << "n10's left child (" << n5->key << ") new color: "
              << RBNode::color_to_string(n10->get_left_color()) << std::endl;
    ASSERT_TRUE(n10->get_left_color() == Color::BLACK, "Color change verification");


    std::cout << "\nExample finished." << std::endl;

    return 0;
}

// #undef ASSERT_TRUE // No longer needed here as it's defined at the top
// #define ASSERT_TRUE(condition, message) \ // No longer needed here
//     if (!(condition)) { \
//         std::cerr << "Assertion failed in example: (" #condition ") is not true - " << (message) << std::endl; \
//         exit(1); \
//     }
