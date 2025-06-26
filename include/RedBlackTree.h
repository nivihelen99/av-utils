#pragma once

#include <iostream>
#include <stdexcept>
#include <memory>
#include <functional> // For std::less

namespace collections {

enum class Color { RED, BLACK };

template <typename Key, typename Value, typename Compare = std::less<Key>>
class RedBlackTree {
public: // Made public for testing access to Node details if needed via getRoot()
    struct Node {
        Key key;
        Value value;
        Color color;
        std::shared_ptr<Node> left;
        std::shared_ptr<Node> right;
        std::weak_ptr<Node> parent; // Use weak_ptr to avoid circular dependencies

        Node(Key k, Value v, Color c)
            : key(k), value(v), color(c), left(nullptr), right(nullptr) {}
    };

    std::shared_ptr<Node> root;
    std::shared_ptr<Node> TNULL; // Sentinel node for leaves
    Compare compare;

    void leftRotate(std::shared_ptr<Node> x) {
        std::shared_ptr<Node> y = x->right;
        x->right = y->left;
        if (y->left != TNULL) {
            y->left->parent = x;
        }
        y->parent = x->parent;
        auto x_parent = x->parent.lock();
        if (!x_parent) {
            root = y;
        } else if (x == x_parent->left) {
            x_parent->left = y;
        } else {
            x_parent->right = y;
        }
        y->left = x;
        x->parent = y;
    }

    void rightRotate(std::shared_ptr<Node> x) {
        std::shared_ptr<Node> y = x->left;
        x->left = y->right;
        if (y->right != TNULL) {
            y->right->parent = x;
        }
        y->parent = x->parent;
        auto x_parent = x->parent.lock();
        if (!x_parent) {
            root = y;
        } else if (x == x_parent->right) {
            x_parent->right = y;
        } else {
            x_parent->left = y;
        }
        y->right = x;
        x->parent = y;
    }

    void fixInsert(std::shared_ptr<Node> k) {
        std::shared_ptr<Node> u;
        while (k != root && k->parent.lock()->color == Color::RED) {
            auto k_parent = k->parent.lock();
            auto k_grandparent = k_parent->parent.lock();
            if (!k_grandparent) {
                break;
            }

            if (k_parent == k_grandparent->left) {
                u = k_grandparent->right;
                if (u != TNULL && u->color == Color::RED) {
                    u->color = Color::BLACK;
                    k_parent->color = Color::BLACK;
                    k_grandparent->color = Color::RED;
                    k = k_grandparent;
                } else {
                    if (k == k_parent->right) {
                        k = k_parent;
                        leftRotate(k);
                        k_parent = k->parent.lock();
                        if(k_parent) k_grandparent = k_parent->parent.lock();
                        else { k = root; break;}
                        if (!k_grandparent) {k = root; break;}
                    }
                    if(k_parent) k_parent->color = Color::BLACK; else {k=root; break;}
                    if(k_grandparent) k_grandparent->color = Color::RED; else {k=root; break;}
                    rightRotate(k_grandparent);
                }
            } else {
                u = k_grandparent->left;
                if (u != TNULL && u->color == Color::RED) {
                    u->color = Color::BLACK;
                    k_parent->color = Color::BLACK;
                    k_grandparent->color = Color::RED;
                    k = k_grandparent;
                } else {
                    if (k == k_parent->left) {
                        k = k_parent;
                        rightRotate(k);
                        k_parent = k->parent.lock();
                        if(k_parent) k_grandparent = k_parent->parent.lock();
                        else { k = root; break; }
                        if (!k_grandparent) {k=root; break;}
                    }
                    if(k_parent) k_parent->color = Color::BLACK; else {k=root; break;}
                    if(k_grandparent) k_grandparent->color = Color::RED; else {k=root; break;}
                    leftRotate(k_grandparent);
                }
            }
            if (k == root) {
                break;
            }
        }
        if (root != TNULL && root != nullptr) root->color = Color::BLACK;
    }

    std::shared_ptr<Node> findNode(std::shared_ptr<Node> node, const Key& key) const {
        while (node != TNULL && node != nullptr && (compare(key, node->key) || compare(node->key, key))) {
            if (compare(key, node->key)) {
                node = node->left;
            } else {
                node = node->right;
            }
        }
        return node;
    }

    void transplant(std::shared_ptr<Node> u, std::shared_ptr<Node> v) {
        auto u_parent = u->parent.lock();
        if (!u_parent) {
            root = v;
        } else if (u == u_parent->left) {
            u_parent->left = v;
        } else {
            u_parent->right = v;
        }
        if (v != TNULL && v != nullptr) {
             v->parent = u->parent;
        }
    }

    std::shared_ptr<Node> minimum(std::shared_ptr<Node> node) {
        while (node != TNULL && node != nullptr && node->left != TNULL && node->left != nullptr) {
            node = node->left;
        }
        return node;
    }

    std::shared_ptr<Node> maximum(std::shared_ptr<Node> node) {
        while (node != TNULL && node != nullptr && node->right != TNULL && node->right != nullptr) {
            node = node->right;
        }
        return node;
    }

    void fixDelete(std::shared_ptr<Node> x) {
        std::shared_ptr<Node> s;
        while (x != root && (x == TNULL || (x != nullptr && x->color == Color::BLACK) ) ) { // x can be TNULL
            // Determine parent of x. If x is TNULL, its parent was temporarily set on TNULL->parent.
            std::shared_ptr<Node> x_parent = (x == TNULL || x == nullptr) ? TNULL->parent.lock() : x->parent.lock();

            if (!x_parent) {
                 break;
            }

            if (x == x_parent->left || (x == TNULL && x_parent->left == TNULL) ) { // x is left child or TNULL is in left slot
                s = x_parent->right;
                if (s == TNULL || s == nullptr) { x = x_parent; continue; } // Sibling is TNULL (black) -> Case 2

                if (s->color == Color::RED) { // Case 1: Sibling s is red
                    s->color = Color::BLACK;
                    x_parent->color = Color::RED;
                    leftRotate(x_parent);
                    s = x_parent->right; // New sibling
                    if (s == TNULL || s == nullptr) { x = root; break; }
                }

                // Now s is black
                bool s_left_black = (s->left == TNULL || s->left == nullptr || s->left->color == Color::BLACK);
                bool s_right_black = (s->right == TNULL || s->right == nullptr || s->right->color == Color::BLACK);

                if (s_left_black && s_right_black) { // Case 2: Sibling s is black, both children black
                    s->color = Color::RED;
                    x = x_parent; // Move problem up
                } else {
                    if (s_right_black) { // Case 3: Sibling s black, s->left red, s->right black
                        if (s->left != TNULL && s->left != nullptr) s->left->color = Color::BLACK;
                        s->color = Color::RED;
                        rightRotate(s);
                        s = x_parent->right; // New sibling
                        if (s == TNULL || s == nullptr) { x = root; break; }
                    }
                    // Case 4: Sibling s black, s->right red
                    s->color = x_parent->color;
                    x_parent->color = Color::BLACK;
                    if (s->right != TNULL && s->right != nullptr) s->right->color = Color::BLACK;
                    leftRotate(x_parent);
                    x = root; // Problem solved
                }
            } else { // Symmetric case: x is right child (or TNULL is in right slot)
                s = x_parent->left;
                 if (s == TNULL || s == nullptr) { x = x_parent; continue; } // Sibling is TNULL (black) -> Case 2

                if (s->color == Color::RED) { // Case 1
                    s->color = Color::BLACK;
                    x_parent->color = Color::RED;
                    rightRotate(x_parent);
                    s = x_parent->left; // New sibling
                    if (s == TNULL || s == nullptr) { x = root; break; }
                }

                bool s_left_black = (s->left == TNULL || s->left == nullptr || s->left->color == Color::BLACK);
                bool s_right_black = (s->right == TNULL || s->right == nullptr || s->right->color == Color::BLACK);

                if (s_left_black && s_right_black) { // Case 2
                    s->color = Color::RED;
                    x = x_parent;
                } else {
                    if (s_left_black) { // Case 3 (symmetric)
                        if (s->right != TNULL && s->right != nullptr) s->right->color = Color::BLACK;
                        s->color = Color::RED;
                        leftRotate(s);
                        s = x_parent->left; // New sibling
                        if (s == TNULL || s == nullptr) { x = root; break; }
                    }
                    // Case 4 (symmetric)
                    s->color = x_parent->color;
                    x_parent->color = Color::BLACK;
                    if (s->left != TNULL && s->left != nullptr) s->left->color = Color::BLACK;
                    rightRotate(x_parent);
                    x = root;
                }
            }
        }
        if (x != TNULL && x != nullptr) {
            x->color = Color::BLACK;
        }
    }

    void deleteNodeHelper(std::shared_ptr<Node> z) {
        std::shared_ptr<Node> y = z;
        std::shared_ptr<Node> x;
        Color y_original_color = y->color;
        std::shared_ptr<Node> parent_of_x_slot_for_tnull;

        if (z->left == TNULL || z->left == nullptr) {
            x = z->right;
            parent_of_x_slot_for_tnull = z->parent.lock();
            transplant(z, x);
        } else if (z->right == TNULL || z->right == nullptr) {
            x = z->left;
            parent_of_x_slot_for_tnull = z->parent.lock();
            transplant(z, x);
        } else {
            y = minimum(z->right);
            y_original_color = y->color;
            x = y->right;

            if (y->parent.lock() == z) {
                parent_of_x_slot_for_tnull = y;
                if (x != TNULL && x != nullptr) {
                    x->parent = y;
                }
            } else {
                parent_of_x_slot_for_tnull = y->parent.lock();
                transplant(y, x);
                y->right = z->right;
                if(y->right != TNULL && y->right != nullptr) y->right->parent = y;
            }
            transplant(z, y);
            y->left = z->left;
            if(y->left != TNULL && y->left != nullptr) y->left->parent = y;
            y->color = z->color;
        }

        std::weak_ptr<Node> old_tnull_parent_wp;
        if (x == TNULL || x == nullptr) { // If x is TNULL or became nullptr
            old_tnull_parent_wp = TNULL->parent; // Save TNULL's original parent (should be null)
            TNULL->parent = parent_of_x_slot_for_tnull; // Point TNULL's parent to where x was, for fixDelete
            x = TNULL; // Ensure x is the canonical TNULL for fixDelete logic
        }

        if (y_original_color == Color::BLACK) {
            fixDelete(x);
        }

        if (x == TNULL) { // If TNULL's parent was modified
            TNULL->parent.reset(); // Reset TNULL's parent
        }
    }

    // For debugging purposes
    void printHelper(std::shared_ptr<Node> node, std::string indent, bool last) const {
        if (node != TNULL && node != nullptr) {
            std::cout << indent;
            if (last) {
                std::cout << "R----";
                indent += "     ";
            } else {
                std::cout << "L----";
                indent += "|    ";
            }

            std::string sColor = node->color == Color::RED ? "RED" : "BLACK";
            std::cout << node->key << "(" << sColor << ")" << std::endl;
            printHelper(node->left, indent, false);
            printHelper(node->right, indent, true);
        }
    }


public:
    RedBlackTree() {
        TNULL = std::make_shared<Node>(Key{}, Value{}, Color::BLACK);
        TNULL->left = nullptr;
        TNULL->right = nullptr;
        root = TNULL;
    }

    void insert(const Key& key, const Value& value) {
        std::shared_ptr<Node> node = std::make_shared<Node>(key, value, Color::RED);
        node->left = TNULL;
        node->right = TNULL;

        std::shared_ptr<Node> y = nullptr;
        std::shared_ptr<Node> x = this->root;

        while (x != TNULL && x != nullptr) {
            y = x;
            if (compare(node->key, x->key)) {
                x = x->left;
            } else if (compare(x->key, node->key)) {
                x = x->right;
            } else {
                x->value = value;
                return;
            }
        }

        node->parent = y;
        if (y == nullptr) {
            root = node;
        } else if (compare(node->key, y->key)) {
            y->left = node;
        } else {
            y->right = node;
        }

        if (node->parent.lock() == nullptr) {
            node->color = Color::BLACK;
            if (root != TNULL && root != nullptr) root->color = Color::BLACK; // Ensure root is black
            return;
        }

        if (node->parent.lock()->color == Color::RED) {
            fixInsert(node);
        }
         if(root != TNULL && root != nullptr) root->color = Color::BLACK;
    }

    Value* find(const Key& key) const {
        std::shared_ptr<Node> node = findNode(this->root, key);
        if (node == TNULL || node == nullptr) {
            return nullptr;
        }
        return &(node->value);
    }

    bool contains(const Key& key) const {
        std::shared_ptr<Node> res = findNode(this->root, key);
        return res != TNULL && res != nullptr;
    }

    void remove(const Key& key) {
        std::shared_ptr<Node> z = findNode(this->root, key);
        if (z == TNULL || z == nullptr) {
            return;
        }
        deleteNodeHelper(z);
         if(root != TNULL && root != nullptr) root->color = Color::BLACK; // Ensure root is black after delete
    }

    bool isEmpty() const {
        return root == TNULL || root == nullptr;
    }

    void printTree() const {
        if (root && root != TNULL && root != nullptr) {
            printHelper(this->root, "", true);
        }
    }

    bool checkProperty2() const {
        return root == TNULL || root == nullptr || root->color == Color::BLACK;
    }

    bool checkProperty4(std::shared_ptr<Node> node) const {
        if (node == TNULL || node == nullptr) return true;
        if (node->color == Color::RED) {
            if ((node->left != TNULL && node->left != nullptr && node->left->color == Color::RED) ||
                (node->right != TNULL && node->right != nullptr && node->right->color == Color::RED)) {
                return false;
            }
        }
        return checkProperty4(node->left) && checkProperty4(node->right);
    }
    bool checkProperty4() const {
        return checkProperty4(root);
    }

    bool checkProperty5(std::shared_ptr<Node> node, int currentBlackCount, int& pathBlackCount) const {
        if (node == TNULL || node == nullptr) {
            currentBlackCount++;
            if (pathBlackCount == -1) {
                pathBlackCount = currentBlackCount;
            }
            return currentBlackCount == pathBlackCount;
        }

        if (node->color == Color::BLACK) {
            currentBlackCount++;
        }

        if (!checkProperty5(node->left, currentBlackCount, pathBlackCount)) return false;
        if (!checkProperty5(node->right, currentBlackCount, pathBlackCount)) return false;

        return true;
    }
    bool checkProperty5() const {
        if (root == TNULL || root == nullptr) return true;
        int pathBlackCount = -1;
        return checkProperty5(root, 0, pathBlackCount);
    }

    std::shared_ptr<Node> getRoot() const { return root; }
    std::shared_ptr<Node> getTNULL() const { return TNULL; }

};

} // namespace collections
