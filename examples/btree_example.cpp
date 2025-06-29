#include "../include/BTree.h" // Adjust path if your BTree.h is elsewhere
#include <iostream>
#include <string>

int main() {
    // Create a B-Tree with integer keys, string values, and a minimum degree of 2.
    // This means each node (except root) has 1 to 3 keys (t-1 to 2t-1)
    // and 2 to 4 children (t to 2t).
    cpp_collections::BTree<int, std::string, std::less<int>, 2> myBTree;

    std::cout << "B-Tree Example" << std::endl;
    std::cout << "Minimum Degree (t): " << myBTree.get_min_degree() << std::endl;
    std::cout << "Initial tree is_empty(): " << (myBTree.is_empty() ? "true" : "false") << std::endl;
    std::cout << "------------------------------------" << std::endl;

    // Insert some key-value pairs
    std::cout << "Inserting elements..." << std::endl;
    myBTree.insert(10, "Apple");
    myBTree.insert(20, "Banana");
    myBTree.insert(5, "Cherry");
    myBTree.insert(15, "Date");
    myBTree.insert(25, "Elderberry");
    myBTree.insert(3, "Fig");       // Causes first split if t=2 (10,20,5 -> 3,5,10,20 -> 5 moves up)
                                     // Actually, with t=2, max keys = 3.
                                     // 10 -> [10]
                                     // 20 -> [10,20]
                                     // 5  -> [5,10,20]
                                     // 15 -> [5,10,15,20] -> root full, split: root [10], L[5], R[15,20]
                                     // 25 -> into R: R[15,20,25]
                                     // 3  -> into L: L[3,5]
    myBTree.insert(30, "Grape");
    myBTree.insert(1, "Kiwi");
    myBTree.insert(7, "Lemon");
    myBTree.insert(12, "Mango");
    myBTree.insert(18, "Nectarine");
    myBTree.insert(22, "Orange");
    myBTree.insert(28, "Papaya");
    myBTree.insert(35, "Quince");


    std::cout << "After insertions, tree is_empty(): " << (myBTree.is_empty() ? "true" : "false") << std::endl;
    std::cout << "------------------------------------" << std::endl;

    // Search for some keys
    std::cout << "Searching for keys..." << std::endl;
    int keysToSearch[] = {10, 5, 20, 15, 25, 3, 30, 99, 7};
    for (int key : keysToSearch) {
        const std::string* value = myBTree.search(key);
        if (value) {
            std::cout << "Found key " << key << ": " << *value << std::endl;
        } else {
            std::cout << "Key " << key << " not found." << std::endl;
        }
    }
    std::cout << "------------------------------------" << std::endl;

    // Example of searching for a non-existent key
    int nonExistentKey = 100;
    const std::string* foundValue = myBTree.search(nonExistentKey);
    if (foundValue) {
        std::cout << "Found non-existent key " << nonExistentKey << ": " << *foundValue << " (Error in test logic or tree)" << std::endl;
    } else {
        std::cout << "Correctly did not find non-existent key " << nonExistentKey << "." << std::endl;
    }
    std::cout << "------------------------------------" << std::endl;

    std::cout << "Inserting more elements to observe structure (if printable)" << std::endl;
    myBTree.insert(40, "Raspberry");
    myBTree.insert(50, "Strawberry");
    myBTree.insert(60, "Tangerine");
    myBTree.insert(70, "Ugli fruit");
    myBTree.insert(80, "Voavanga");

    // Search again after more insertions
    std::cout << "Searching for 60: " << (myBTree.search(60) ? *myBTree.search(60) : "Not Found") << std::endl;
    std::cout << "Searching for 80: " << (myBTree.search(80) ? *myBTree.search(80) : "Not Found") << std::endl;

    // Note: To visually inspect the B-Tree structure, you would typically need
    // a print or traversal function within the BTree class itself.
    // This example focuses on the public API: insert and search.

    std::cout << "------------------------------------" << std::endl;
    std::cout << "B-Tree example finished." << std::endl;

    return 0;
}
