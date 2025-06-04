#include <iostream>
#include <vector>
#include <random>
#include <climits>
#include <iomanip>

template<typename T>
class SkipListNode {
public:
    T value;
    std::vector<SkipListNode*> forward;
    
    SkipListNode(T val, int level) : value(val), forward(level + 1, nullptr) {}
};

template<typename T>
class SkipList {
private:
    static const int MAX_LEVEL = 16;
    SkipListNode<T>* header;
    int currentLevel;
    std::mt19937 rng;
    std::uniform_real_distribution<double> dist;
    
public:
    SkipList() : currentLevel(0), rng(std::random_device{}()), dist(0.0, 1.0) {
        // Create header node with minimum possible value
        header = new SkipListNode<T>(T{}, MAX_LEVEL);
    }
    
    ~SkipList() {
        SkipListNode<T>* current = header;
        while (current != nullptr) {
            SkipListNode<T>* next = current->forward[0];
            delete current;
            current = next;
        }
    }
    
    // Generate random level for new node (geometric distribution)
    int randomLevel() {
        int level = 0;
        while (dist(rng) < 0.5 && level < MAX_LEVEL) {
            level++;
        }
        return level;
    }
    
    // Insert a value into the skip list
    void insert(T value) {
        std::vector<SkipListNode<T>*> update(MAX_LEVEL + 1, nullptr);
        SkipListNode<T>* current = header;
        
        // Find position to insert
        for (int i = currentLevel; i >= 0; i--) {
            while (current->forward[i] != nullptr && current->forward[i]->value < value) {
                current = current->forward[i];
            }
            update[i] = current;
        }
        
        current = current->forward[0];
        
        // If value doesn't exist, insert it
        if (current == nullptr || current->value != value) {
            int newLevel = randomLevel();
            
            // If new level is higher than current level, update header pointers
            if (newLevel > currentLevel) {
                for (int i = currentLevel + 1; i <= newLevel; i++) {
                    update[i] = header;
                }
                currentLevel = newLevel;
            }
            
            // Create new node
            SkipListNode<T>* newNode = new SkipListNode<T>(value, newLevel);
            
            // Update forward pointers
            for (int i = 0; i <= newLevel; i++) {
                newNode->forward[i] = update[i]->forward[i];
                update[i]->forward[i] = newNode;
            }
            
            std::cout << "Inserted " << value << " at level " << newLevel << std::endl;
        } else {
            std::cout << "Value " << value << " already exists" << std::endl;
        }
    }
    
    // Search for a value in the skip list
    bool search(T value) const {
        SkipListNode<T>* current = header;
        
        // Start from highest level and go down
        for (int i = currentLevel; i >= 0; i--) {
            while (current->forward[i] != nullptr && current->forward[i]->value < value) {
                current = current->forward[i];
            }
        }
        
        current = current->forward[0];
        return (current != nullptr && current->value == value);
    }
    
    // Delete a value from the skip list
    bool remove(T value) {
        std::vector<SkipListNode<T>*> update(MAX_LEVEL + 1, nullptr);
        SkipListNode<T>* current = header;
        
        // Find the node to delete
        for (int i = currentLevel; i >= 0; i--) {
            while (current->forward[i] != nullptr && current->forward[i]->value < value) {
                current = current->forward[i];
            }
            update[i] = current;
        }
        
        current = current->forward[0];
        
        // If found, delete it
        if (current != nullptr && current->value == value) {
            // Update forward pointers
            for (int i = 0; i <= currentLevel; i++) {
                if (update[i]->forward[i] != current) {
                    break;
                }
                update[i]->forward[i] = current->forward[i];
            }
            
            delete current;
            
            // Update current level if necessary
            while (currentLevel > 0 && header->forward[currentLevel] == nullptr) {
                currentLevel--;
            }
            
            std::cout << "Deleted " << value << std::endl;
            return true;
        }
        
        std::cout << "Value " << value << " not found for deletion" << std::endl;
        return false;
    }
    
    // Display the skip list structure
    void display() const {
        std::cout << "\n=== Skip List Structure ===" << std::endl;
        
        for (int i = currentLevel; i >= 0; i--) {
            std::cout << "Level " << std::setw(2) << i << ": ";
            SkipListNode<T>* current = header->forward[i];
            
            while (current != nullptr) {
                std::cout << std::setw(4) << current->value << " -> ";
                current = current->forward[i];
            }
            std::cout << "NULL" << std::endl;
        }
        std::cout << std::endl;
    }
    
    // Print all values in sorted order
    void printValues() const {
        std::cout << "Values in skip list: ";
        SkipListNode<T>* current = header->forward[0];
        
        while (current != nullptr) {
            std::cout << current->value << " ";
            current = current->forward[0];
        }
        std::cout << std::endl;
    }
    
    // Get the size of the skip list
    int size() const {
        int count = 0;
        SkipListNode<T>* current = header->forward[0];
        
        while (current != nullptr) {
            count++;
            current = current->forward[0];
        }
        return count;
    }
    
    // Find the k-th smallest element (0-indexed)
    T kthElement(int k) const {
        if (k < 0) {
            throw std::invalid_argument("k must be non-negative");
        }
        
        SkipListNode<T>* current = header->forward[0];
        
        for (int i = 0; i < k && current != nullptr; i++) {
            current = current->forward[0];
        }
        
        if (current == nullptr) {
            throw std::out_of_range("k is larger than skip list size");
        }
        
        return current->value;
    }
    
    // Find elements in a range [min, max]
    std::vector<T> rangeQuery(T minVal, T maxVal) const {
        std::vector<T> result;
        SkipListNode<T>* current = header;
        
        // Find the starting position
        for (int i = currentLevel; i >= 0; i--) {
            while (current->forward[i] != nullptr && current->forward[i]->value < minVal) {
                current = current->forward[i];
            }
        }
        
        current = current->forward[0];
        
        // Collect values in range
        while (current != nullptr && current->value <= maxVal) {
            if (current->value >= minVal) {
                result.push_back(current->value);
            }
            current = current->forward[0];
        }
        
        return result;
    }
};

