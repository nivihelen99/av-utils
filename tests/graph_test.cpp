#include "Graph.h"
#include "gtest/gtest.h"
#include <string>
#include <vector>
#include <algorithm> // For std::is_permutation, std::find
#include <set>       // For comparing node sets

// Helper to check if a path exists in a sorted list
template <typename NodeID>
bool check_order(const std::vector<NodeID>& sorted_list, const NodeID& before, const NodeID& after) {
    auto it_before = std::find(sorted_list.begin(), sorted_list.end(), before);
    auto it_after = std::find(sorted_list.begin(), sorted_list.end(), after);

    if (it_before == sorted_list.end() || it_after == sorted_list.end()) {
        // If one of the nodes isn't even in the list, the order can't be satisfied
        // This might happen if the graph structure implies they should be there but topo sort fails for other reasons
        return false;
    }
    return it_before < it_after;
}


TEST(GraphTest, Initialization) {
    cpp_utils::Graph<int> g;
    ASSERT_EQ(g.num_nodes(), 0);
    ASSERT_EQ(g.num_edges(), 0);
}

TEST(GraphTest, AddNode) {
    cpp_utils::Graph<std::string> g;
    ASSERT_TRUE(g.add_node("A"));
    ASSERT_EQ(g.num_nodes(), 1);
    ASSERT_TRUE(g.has_node("A"));
    ASSERT_FALSE(g.has_node("B"));

    ASSERT_FALSE(g.add_node("A")); // Add duplicate
    ASSERT_EQ(g.num_nodes(), 1);

    g.add_node("B");
    ASSERT_EQ(g.num_nodes(), 2);
    ASSERT_TRUE(g.has_node("B"));

    auto all_nodes = g.get_all_nodes();
    std::set<std::string> node_set(all_nodes.begin(), all_nodes.end());
    ASSERT_EQ(node_set.size(), 2);
    ASSERT_TRUE(node_set.count("A"));
    ASSERT_TRUE(node_set.count("B"));
}

TEST(GraphTest, AddEdge) {
    cpp_utils::Graph<int> g;
    g.add_edge(1, 2);
    ASSERT_EQ(g.num_nodes(), 2); // Nodes should be implicitly created
    ASSERT_EQ(g.num_edges(), 1);
    ASSERT_TRUE(g.has_node(1));
    ASSERT_TRUE(g.has_node(2));

    const auto& neighbors_of_1 = g.get_neighbors(1);
    ASSERT_EQ(neighbors_of_1.size(), 1);
    ASSERT_TRUE(neighbors_of_1.count(2));

    const auto& neighbors_of_2 = g.get_neighbors(2);
    ASSERT_TRUE(neighbors_of_2.empty());

    g.add_edge(1, 3); // Add another edge from 1
    ASSERT_EQ(g.num_nodes(), 3);
    ASSERT_EQ(g.num_edges(), 2);
    ASSERT_TRUE(g.get_neighbors(1).count(3));

    g.add_edge(1, 2); // Add duplicate edge
    ASSERT_EQ(g.num_edges(), 2); // Edge count should not change due to set uniqueness
    ASSERT_EQ(g.get_neighbors(1).size(), 2);


    g.add_edge(2, 3);
    ASSERT_EQ(g.num_nodes(), 3); // No new node
    ASSERT_EQ(g.num_edges(), 3);
    ASSERT_TRUE(g.get_neighbors(2).count(3));
}

TEST(GraphTest, GetNeighborsException) {
    cpp_utils::Graph<int> g;
    g.add_node(1);
    ASSERT_THROW(g.get_neighbors(2), std::out_of_range);
    ASSERT_NO_THROW(g.get_neighbors(1));
}


TEST(GraphTest, GetAllNodes) {
    cpp_utils::Graph<char> g;
    g.add_node('A');
    g.add_edge('B', 'C'); // B and C are added
    g.add_node('D');

    auto nodes_vec = g.get_all_nodes();
    std::set<char> nodes_set(nodes_vec.begin(), nodes_vec.end());

    ASSERT_EQ(nodes_set.size(), 4);
    ASSERT_TRUE(nodes_set.count('A'));
    ASSERT_TRUE(nodes_set.count('B'));
    ASSERT_TRUE(nodes_set.count('C'));
    ASSERT_TRUE(nodes_set.count('D'));
}


TEST(TopologicalSort, EmptyGraph) {
    cpp_utils::Graph<int> g;
    auto result = g.topological_sort();
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result.value().empty());
}

TEST(TopologicalSort, SingleNodeGraph) {
    cpp_utils::Graph<std::string> g;
    g.add_node("lonely");
    auto result = g.topological_sort();
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value().size(), 1);
    ASSERT_EQ(result.value()[0], "lonely");
}

TEST(TopologicalSort, SimpleLinearGraph) {
    cpp_utils::Graph<int> g;
    // 3 -> 2 -> 1 -> 0
    g.add_edge(3, 2);
    g.add_edge(2, 1);
    g.add_edge(1, 0);

    auto result = g.topological_sort();
    ASSERT_TRUE(result.has_value());
    const auto& sorted = result.value();
    ASSERT_EQ(sorted.size(), 4);
    ASSERT_TRUE(check_order(sorted, 3, 2));
    ASSERT_TRUE(check_order(sorted, 2, 1));
    ASSERT_TRUE(check_order(sorted, 1, 0));
    // Exact order should be 3, 2, 1, 0
    ASSERT_EQ(sorted[0], 3);
    ASSERT_EQ(sorted[1], 2);
    ASSERT_EQ(sorted[2], 1);
    ASSERT_EQ(sorted[3], 0);
}

TEST(TopologicalSort, MultipleInitialNodes) {
    cpp_utils::Graph<std::string> g;
    // A -> C
    // B -> C
    // B -> D
    // C -> E
    // D -> E
    // F (isolated)
    g.add_edge("A", "C");
    g.add_edge("B", "C");
    g.add_edge("B", "D");
    g.add_edge("C", "E");
    g.add_edge("D", "E");
    g.add_node("F"); // No edges

    auto result = g.topological_sort();
    ASSERT_TRUE(result.has_value());
    const auto& sorted = result.value();
    ASSERT_EQ(sorted.size(), 6);

    // Check all nodes are present
    std::set<std::string> sorted_set(sorted.begin(), sorted.end());
    ASSERT_TRUE(sorted_set.count("A"));
    ASSERT_TRUE(sorted_set.count("B"));
    ASSERT_TRUE(sorted_set.count("C"));
    ASSERT_TRUE(sorted_set.count("D"));
    ASSERT_TRUE(sorted_set.count("E"));
    ASSERT_TRUE(sorted_set.count("F"));

    // Check dependencies
    ASSERT_TRUE(check_order(sorted, std::string("A"), std::string("C")));
    ASSERT_TRUE(check_order(sorted, std::string("B"), std::string("C")));
    ASSERT_TRUE(check_order(sorted, std::string("B"), std::string("D")));
    ASSERT_TRUE(check_order(sorted, std::string("C"), std::string("E")));
    ASSERT_TRUE(check_order(sorted, std::string("D"), std::string("E")));

    // Initial nodes (A, B, F) can appear in any order relative to each other at the start
    // E must be last or among the last
    ASSERT_EQ(sorted.back(), "E"); // E is the ultimate sink here
}


TEST(TopologicalSort, DisconnectedComponentsDAG) {
    cpp_utils::Graph<char> g;
    g.add_edge('A', 'B'); // Component 1: A -> B
    g.add_edge('X', 'Y'); // Component 2: X -> Y
    g.add_node('M');      // Component 3: M (isolated)

    auto result = g.topological_sort();
    ASSERT_TRUE(result.has_value());
    const auto& sorted = result.value();
    ASSERT_EQ(sorted.size(), 5);

    std::set<char> sorted_set(sorted.begin(), sorted.end());
    ASSERT_TRUE(sorted_set.count('A'));
    ASSERT_TRUE(sorted_set.count('B'));
    ASSERT_TRUE(sorted_set.count('X'));
    ASSERT_TRUE(sorted_set.count('Y'));
    ASSERT_TRUE(sorted_set.count('M'));

    ASSERT_TRUE(check_order(sorted, 'A', 'B'));
    ASSERT_TRUE(check_order(sorted, 'X', 'Y'));
}

TEST(TopologicalSort, GraphWithCycle) {
    cpp_utils::Graph<int> g;
    g.add_edge(1, 2);
    g.add_edge(2, 3);
    g.add_edge(3, 1); // Cycle: 1 -> 2 -> 3 -> 1
    g.add_edge(3, 4); // Edge outside the cycle

    auto result = g.topological_sort();
    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(result.error(), "Graph has a cycle, topological sort not possible.");
}

TEST(TopologicalSort, ComplexDAG) {
    // From Cormen et al. (Introduction to Algorithms) example for topological sort
    //เสื้อ -> กางเกง -> เข็มขัด -> แจ็คเก็ต
    //      -> เนคไท -> แจ็คเก็ต
    //นาฬิกา (no dependencies, no dependents in this chain)
    //ถุงเท้า -> รองเท้า
    //กางเกงใน -> กางเกง
    //         -> รองเท้า
    cpp_utils::Graph<std::string> g;
    g.add_edge("undershorts", "pants");
    g.add_edge("undershorts", "shoes");
    g.add_edge("pants", "belt");
    g.add_edge("pants", "shoes"); // You typically put on pants before shoes
    g.add_edge("belt", "jacket");
    g.add_edge("shirt", "belt");
    g.add_edge("shirt", "tie");
    g.add_edge("tie", "jacket");
    g.add_edge("socks", "shoes");
    g.add_node("watch"); // No dependencies, no one depends on it in this example

    auto result = g.topological_sort();
    ASSERT_TRUE(result.has_value());
    const auto& sorted = result.value();

    // All items must be present
    ASSERT_EQ(sorted.size(), 9);
    std::set<std::string> sorted_set(sorted.begin(), sorted.end());
    ASSERT_TRUE(sorted_set.count("undershorts"));
    ASSERT_TRUE(sorted_set.count("pants"));
    ASSERT_TRUE(sorted_set.count("belt"));
    ASSERT_TRUE(sorted_set.count("jacket"));
    ASSERT_TRUE(sorted_set.count("shirt"));
    ASSERT_TRUE(sorted_set.count("tie"));
    ASSERT_TRUE(sorted_set.count("socks"));
    ASSERT_TRUE(sorted_set.count("shoes"));
    ASSERT_TRUE(sorted_set.count("watch"));


    // Check key dependencies
    ASSERT_TRUE(check_order(sorted, std::string("undershorts"), std::string("pants")));
    ASSERT_TRUE(check_order(sorted, std::string("undershorts"), std::string("shoes")));
    ASSERT_TRUE(check_order(sorted, std::string("pants"), std::string("belt")));
    ASSERT_TRUE(check_order(sorted, std::string("pants"), std::string("shoes")));
    ASSERT_TRUE(check_order(sorted, std::string("belt"), std::string("jacket")));
    ASSERT_TRUE(check_order(sorted, std::string("shirt"), std::string("belt")));
    ASSERT_TRUE(check_order(sorted, std::string("shirt"), std::string("tie")));
    ASSERT_TRUE(check_order(sorted, std::string("tie"), std::string("jacket")));
    ASSERT_TRUE(check_order(sorted, std::string("socks"), std::string("shoes")));

    // Jacket and shoes are common sinks/later items
    auto jacket_pos = std::find(sorted.begin(), sorted.end(), std::string("jacket"));
    auto shoes_pos = std::find(sorted.begin(), sorted.end(), std::string("shoes"));
    ASSERT_NE(jacket_pos, sorted.end());
    ASSERT_NE(shoes_pos, sorted.end());
}


TEST(TopologicalSort, SelfLoopCycle) {
    cpp_utils::Graph<int> g;
    g.add_edge(1, 1); // Self-loop
    g.add_edge(0, 1);
    g.add_edge(1, 2);

    auto result = g.topological_sort();
    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(result.error(), "Graph has a cycle, topological sort not possible.");
}

TEST(TopologicalSort, TwoNodeCycle) {
    cpp_utils::Graph<char> g;
    g.add_edge('A', 'B');
    g.add_edge('B', 'A'); // A <-> B cycle
    g.add_edge('C', 'A'); // C points into cycle

    auto result = g.topological_sort();
    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(result.error(), "Graph has a cycle, topological sort not possible.");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
