#include "../include/BTree.h" // Adjust path as necessary
#include <iostream>
#include <cassert>
#include <string>
#include <vector>

// Basic test function
void test_simple_insert_and_search() {
    std::cout << "--- Test: Simple Insert and Search ---" << std::endl;
    cpp_collections::BTree<int, std::string, std::less<int>, 2> tree; // MinDegree t=2

    tree.insert(10, "Value10");
    tree.insert(20, "Value20");
    tree.insert(5, "Value5");

    std::cout << "Searching for 10: " << (tree.search(10) ? *tree.search(10) : "Not Found") << std::endl;
    assert(tree.search(10) != nullptr && *tree.search(10) == "Value10");

    std::cout << "Searching for 20: " << (tree.search(20) ? *tree.search(20) : "Not Found") << std::endl;
    assert(tree.search(20) != nullptr && *tree.search(20) == "Value20");

    std::cout << "Searching for 5: " << (tree.search(5) ? *tree.search(5) : "Not Found") << std::endl;
    assert(tree.search(5) != nullptr && *tree.search(5) == "Value5");

    std::cout << "Searching for 15 (non-existent): " << (tree.search(15) ? *tree.search(15) : "Not Found") << std::endl;
    assert(tree.search(15) == nullptr);

    std::cout << "Simple Insert and Search Test Passed!" << std::endl;
}

void test_root_split() {
    std::cout << "--- Test: Root Split ---" << std::endl;
    // MinDegree t=2 means max keys in a node is 2*t - 1 = 3.
    // Insertion of 4th key into a single node tree will cause root split.
    cpp_collections::BTree<int, int, std::less<int>, 2> tree;

    tree.insert(10, 100); // Root: [10]
    tree.insert(20, 200); // Root: [10, 20]
    tree.insert(30, 300); // Root: [10, 20, 30] - Full

    std::cout << "Before root split (inserted 10, 20, 30)" << std::endl;
    assert(*tree.search(10) == 100);
    assert(*tree.search(20) == 200);
    assert(*tree.search(30) == 300);

    tree.insert(15, 150); // This should cause a root split. Middle key (20) moves up.
                          // New Root: [20]
                          // Left Child: [10, 15]
                          // Right Child: [30]

    std::cout << "After inserting 15 (should trigger root split)" << std::endl;
    assert(tree.search(10) != nullptr && *tree.search(10) == 100);
    assert(tree.search(15) != nullptr && *tree.search(15) == 150);
    assert(tree.search(20) != nullptr && *tree.search(20) == 200);
    assert(tree.search(30) != nullptr && *tree.search(30) == 300);

    // Verify structure (conceptually) - actual check needs access to root/children or a print function
    // For t=2, root should now have 1 key (20).
    // Its children should contain 10,15 and 30.

    tree.insert(5, 50);
    tree.insert(25, 250);
    tree.insert(35, 350);
    tree.insert(1, 10);
    tree.insert(12, 120);
    tree.insert(17, 170);
    tree.insert(22, 220);
    tree.insert(27, 270);
    tree.insert(32, 320);
    tree.insert(37, 370);


    std::cout << "Searching after more insertions:" << std::endl;
    assert(tree.search(5) != nullptr && *tree.search(5) == 50);
    assert(tree.search(25) != nullptr && *tree.search(25) == 250);
    assert(tree.search(35) != nullptr && *tree.search(35) == 350);
    assert(tree.search(1) != nullptr && *tree.search(1) == 10);
    assert(tree.search(12) != nullptr && *tree.search(12) == 120);
    assert(tree.search(17) != nullptr && *tree.search(17) == 170);
    assert(tree.search(22) != nullptr && *tree.search(22) == 220);
    assert(tree.search(27) != nullptr && *tree.search(27) == 270);
    assert(tree.search(32) != nullptr && *tree.search(32) == 320);
    assert(tree.search(37) != nullptr && *tree.search(37) == 370);
    assert(tree.search(99) == nullptr);


    std::cout << "Root Split Test Passed!" << std::endl;
}

void test_internal_node_split() {
    std::cout << "--- Test: Internal Node Split ---" << std::endl;
    // MinDegree t=2. Max keys = 3.
    // We need to create a situation where an internal node (not the root) becomes full and splits.
    cpp_collections::BTree<int, std::string, std::less<int>, 2> tree;

    // Phase 1: Cause a root split to get some depth
    tree.insert(10, "10");
    tree.insert(20, "20");
    tree.insert(30, "30"); // Root: [10,20,30]
    tree.insert(15, "15"); // Root splits. Root: [20], L: [10,15], R: [30]

    std::cout << "After initial root split:" << std::endl;
    assert(*tree.search(20) == "20");
    assert(*tree.search(10) == "10");
    assert(*tree.search(15) == "15");
    assert(*tree.search(30) == "30");

    // Phase 2: Fill one of the children of the root to force it to split
    // Let's fill the left child L: [10,15]. It needs 2 more keys to be full for t=2 (max 3 keys).
    // Current state: Root: [20], L:[10,15], R:[30]
    tree.insert(5, "5");  // Goes to L. L: [5,10,15] - L is now full
    std::cout << "Inserted 5. Left child should be [5,10,15]" << std::endl;
    assert(*tree.search(5) == "5");

    // Now, insert a key that goes into L, causing L to split.
    // For example, insert 7.
    // L ([5,10,15]) splits. Median 10 moves up to root.
    // Root becomes: [10,20]
    // Child1 (new L): [5,7]
    // Child2 (new middle from old L's split): [15]
    // Child3 (old R): [30]
    tree.insert(7, "7");
    std::cout << "Inserted 7. Left child [5,10,15] should split. Median 10 moves to root." << std::endl;

    assert(tree.search(5) != nullptr && *tree.search(5) == "5");
    assert(tree.search(7) != nullptr && *tree.search(7) == "7");
    assert(tree.search(10) != nullptr && *tree.search(10) == "10"); // Key 10 is now in root
    assert(tree.search(15) != nullptr && *tree.search(15) == "15");
    assert(tree.search(20) != nullptr && *tree.search(20) == "20"); // Key 20 is still in root
    assert(tree.search(30) != nullptr && *tree.search(30) == "30");

    // Insert more to test further splits
    tree.insert(1, "1");
    tree.insert(12, "12");
    tree.insert(17, "17");
    tree.insert(25, "25");
    tree.insert(27, "27"); // This might cause the node containing 30 to split if it fills up.
                           // Root: [10,20]. Child3 (old R) was [30].
                           // Insert 25: goes to Child3. Child3: [25,30]
                           // Insert 27: goes to Child3. Child3: [25,27,30] - Full.
                           // Insert 22: should go to child of 20 where 25 is.
                           // Let's trace: inserting 22.
                           // Root [10,20]. k=22 > 10. k=22 <=20 (false). So k > 20. Descend right of 20.
                           // Child is [30]. (Mistake here, child R should be [30] initially).
                           // After 5,7,10... Root: [10,20]. Children: C1:[5,7], C2:[15], C3:[30]
                           // Insert 25: Root [10,20]. k=25. Descend to C3. C3 is leaf [30]. Insert 25. C3 becomes [25,30].
                           // Insert 27: Root [10,20]. k=27. Descend to C3. C3 is leaf [25,30]. Insert 27. C3 becomes [25,27,30]. (Full)
                           // Insert 35: Root [10,20]. k=35. Descend to C3 [25,27,30]. C3 is full. SPLIT C3.
                           // Median 27 moves up. Root: [10,20,27].
                           // C3 (old) becomes [25]. New sibling C4 becomes [30].
                           // Now insert 35. Root [10,20,27]. k=35 > 27. Descend to C4 [30]. Insert 35. C4 becomes [30,35].

    tree.insert(35,"35");

    std::cout << "After more insertions (1,12,17,25,27,35)" << std::endl;
    assert(*tree.search(1) == "1");
    assert(*tree.search(12) == "12");
    assert(*tree.search(17) == "17");
    assert(*tree.search(25) == "25");
    assert(*tree.search(27) == "27"); // Should be in root
    assert(*tree.search(35) == "35");


    std::cout << "Internal Node Split Test Passed!" << std::endl;
}


void test_larger_degree() {
    std::cout << "--- Test: Larger Degree (t=3) ---" << std::endl;
    // MinDegree t=3 means max keys in a node is 2*t - 1 = 5.
    // A node splits when it has 5 keys and a 6th is inserted.
    cpp_collections::BTree<int, int, std::less<int>, 3> tree; // t=3

    // Insert 5 keys, node should be full
    tree.insert(10, 100);
    tree.insert(20, 200);
    tree.insert(30, 300);
    tree.insert(40, 400);
    tree.insert(50, 500); // Root: [10,20,30,40,50] - Full

    assert(tree.search(10) && *tree.search(10) == 100);
    assert(tree.search(50) && *tree.search(50) == 500);

    // Insert 6th key, should cause root split
    tree.insert(25, 250); // Median key (30) moves up.
                          // New Root: [30]
                          // L: [10,20,25] R: [40,50]

    std::cout << "After inserting 25 (should trigger root split for t=3)" << std::endl;
    assert(tree.search(10) && *tree.search(10) == 100);
    assert(tree.search(20) && *tree.search(20) == 200);
    assert(tree.search(25) && *tree.search(25) == 250);
    assert(tree.search(30) && *tree.search(30) == 300); // Root
    assert(tree.search(40) && *tree.search(40) == 400);
    assert(tree.search(50) && *tree.search(50) == 500);

    // Fill up L child: [10,20,25]. Needs 2 more to be full (max 5 keys).
    tree.insert(5, 50);
    tree.insert(15, 150); // L is now [5,10,15,20,25] - Full

    assert(tree.search(5) && *tree.search(5) == 50);
    assert(tree.search(15) && *tree.search(15) == 150);

    // Insert into L to make it split
    tree.insert(12, 120); // L ([5,10,15,20,25]) splits. Median (15) moves up.
                          // Root: [15,30]
                          // C1: [5,10,12] C2: [20,25] C3: [40,50]

    std::cout << "After inserting 12 (should trigger internal split for t=3)" << std::endl;
    assert(tree.search(5) && *tree.search(5) == 50);
    assert(tree.search(10) && *tree.search(10) == 100);
    assert(tree.search(12) && *tree.search(12) == 120);
    assert(tree.search(15) && *tree.search(15) == 150); // In root
    assert(tree.search(20) && *tree.search(20) == 200);
    assert(tree.search(25) && *tree.search(25) == 250);
    assert(tree.search(30) && *tree.search(30) == 300); // In root
    assert(tree.search(40) && *tree.search(40) == 400);
    assert(tree.search(50) && *tree.search(50) == 500);


    std::cout << "Larger Degree Test Passed!" << std::endl;
}


int main() {
    test_simple_insert_and_search();
    std::cout << std::endl;
    test_root_split();
    std::cout << std::endl;
    test_internal_node_split();
    std::cout << std::endl;
    test_larger_degree();
    std::cout << std::endl;

    std::cout << "All B-Tree tests completed." << std::endl;
    return 0;
}
