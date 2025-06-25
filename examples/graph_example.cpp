#include "Graph.h"
#include <iostream>
#include <string>
#include <vector>

// Helper function to print the result of topological sort
template <typename NodeID>
void print_topo_sort_result(const cpp_utils::Graph<NodeID>& graph, const std::string& graph_name) {
    std::cout << "--- Topological Sort for " << graph_name << " ---" << std::endl;
    auto result = graph.topological_sort();

    if (result) {
        std::cout << "Sorted order: ";
        for (const auto& node : *result) {
            std::cout << node << " ";
        }
        std::cout << std::endl;
    } else {
        std::cout << "Error: " << result.error() << std::endl;
    }
    std::cout << "-----------------------------------------" << std::endl << std::endl;
}

int main() {
    // Example 1: Simple DAG with integer node IDs
    cpp_utils::Graph<int> graph1;
    graph1.add_edge(5, 2);
    graph1.add_edge(5, 0);
    graph1.add_edge(4, 0);
    graph1.add_edge(4, 1);
    graph1.add_edge(2, 3);
    graph1.add_edge(3, 1);
    // Expected: 4, 5, 0, 2, 3, 1 (or other valid orders like 5, 4, 2, 0, 3, 1)

    std::cout << "Graph 1 (Integers):" << std::endl;
    std::cout << "Nodes: " << graph1.num_nodes() << ", Edges: " << graph1.num_edges() << std::endl;
    for(const auto& node : graph1.get_all_nodes()) {
        std::cout << "Node " << node << " neighbors: ";
        for(const auto& neighbor : graph1.get_neighbors(node)) {
            std::cout << neighbor << " ";
        }
        std::cout << std::endl;
    }
    print_topo_sort_result(graph1, "Graph 1 (Integers)");


    // Example 2: Graph with a cycle (integer node IDs)
    cpp_utils::Graph<int> graph2;
    graph2.add_edge(1, 2);
    graph2.add_edge(2, 3);
    graph2.add_edge(3, 1); // Cycle 1 -> 2 -> 3 -> 1
    graph2.add_edge(3, 4);
    print_topo_sort_result(graph2, "Graph 2 (Cycle with Integers)");


    // Example 3: DAG with string node IDs (e.g., task dependencies)
    cpp_utils::Graph<std::string> task_graph;
    task_graph.add_edge("laundry", "drying");
    task_graph.add_edge("groceries", "cooking");
    task_graph.add_edge("cooking", "dishes");
    task_graph.add_edge("drying", "folding");
    task_graph.add_edge("laundry", "folding"); // Can fold directly after laundry if not drying (e.g. hand wash)
                                            // This makes "folding" depend on "laundry" and "drying"
                                            // if "drying" itself depends on "laundry"
    task_graph.add_node("vacuuming"); // A task with no dependencies and no dependents initially

    // Let's make "folding" depend on "drying" which depends on "laundry"
    // "laundry" -> "drying" -> "folding"
    // "groceries" -> "cooking" -> "dishes"
    // "vacuuming"
    // Expected: e.g. laundry, groceries, vacuuming, drying, cooking, folding, dishes (order of independent items can vary)

    print_topo_sort_result(task_graph, "Task Graph (Strings)");

    // Example 4: Graph with disconnected components (still a DAG)
    cpp_utils::Graph<char> graph4;
    graph4.add_edge('A', 'B');
    graph4.add_edge('C', 'D');
    graph4.add_node('E');
    // Expected: A, C, E, B, D (order of A,C,E can vary, B after A, D after C)
    print_topo_sort_result(graph4, "Graph 4 (Disconnected Components)");

    // Example 5: Empty graph
    cpp_utils::Graph<int> empty_graph;
    print_topo_sort_result(empty_graph, "Empty Graph");

    // Example 6: Single node graph
    cpp_utils::Graph<std::string> single_node_graph;
    single_node_graph.add_node("lonely_node");
    print_topo_sort_result(single_node_graph, "Single Node Graph");

    std::cout << "Example run complete." << std::endl;

    return 0;
}
