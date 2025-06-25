#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <stdexcept> // For std::runtime_error (though we'll aim for expected later)
#include <algorithm> // For std::find, std::remove (if needed)
#include "expected.h" // For topological_sort return type

namespace cpp_utils {

template <typename NodeID>
class Graph {
public:
    Graph() = default;

    // Adds a node to the graph.
    // Returns true if the node was added, false if it already existed.
    bool add_node(const NodeID& node_id) {
        if (adj_list_.count(node_id)) {
            return false; // Node already exists
        }
        adj_list_[node_id] = std::unordered_set<NodeID>();
        return true;
    }

    // Adds a directed edge from 'from_node_id' to 'to_node_id'.
    // If either node does not exist, it will be created.
    void add_edge(const NodeID& from_node_id, const NodeID& to_node_id) {
        add_node(from_node_id); // Ensure 'from' node exists
        add_node(to_node_id);   // Ensure 'to' node exists
        adj_list_[from_node_id].insert(to_node_id);
    }

    // Gets the set of neighbors for a given node.
    // Throws std::out_of_range if the node does not exist.
    const std::unordered_set<NodeID>& get_neighbors(const NodeID& node_id) const {
        auto it = adj_list_.find(node_id);
        if (it == adj_list_.end()) {
            throw std::out_of_range("Node not found in graph");
        }
        return it->second;
    }

    // Gets the set of neighbors for a given node (non-const version).
    // Throws std::out_of_range if the node does not exist.
    std::unordered_set<NodeID>& get_neighbors(const NodeID& node_id) {
        auto it = adj_list_.find(node_id);
        if (it == adj_list_.end()) {
            throw std::out_of_range("Node not found in graph");
        }
        return it->second;
    }

    // Checks if a node exists in the graph.
    bool has_node(const NodeID& node_id) const {
        return adj_list_.count(node_id);
    }

    // Gets a vector of all node IDs in the graph.
    // The order is not guaranteed.
    std::vector<NodeID> get_all_nodes() const {
        std::vector<NodeID> nodes;
        nodes.reserve(adj_list_.size());
        for (const auto& pair : adj_list_) {
            nodes.push_back(pair.first);
        }
        return nodes;
    }

    // Gets the number of nodes in the graph.
    size_t num_nodes() const {
        return adj_list_.size();
    }

    // Gets the number of edges in the graph.
    size_t num_edges() const {
        size_t count = 0;
        for (const auto& pair : adj_list_) {
            count += pair.second.size();
        }
        return count;
    }

    // Performs a topological sort on the graph.
    // Returns a vector of NodeIDs in topologically sorted order.
    // If the graph contains a cycle, returns an error message via aos_utils::Expected.
    aos_utils::Expected<std::vector<NodeID>, std::string> topological_sort() const {
        std::vector<NodeID> sorted_order;
        sorted_order.reserve(num_nodes());

        std::unordered_map<NodeID, int> in_degree;
        std::vector<NodeID> queue; // Using std::vector as a queue

        // Initialize in-degrees for all nodes
        for (const auto& pair : adj_list_) {
            in_degree[pair.first] = 0;
        }

        // Calculate in-degrees
        for (const auto& pair : adj_list_) {
            for (const NodeID& neighbor : pair.second) {
                // Ensure neighbor is in the in_degree map, even if it has no outgoing edges itself
                // (though add_node during add_edge should have already ensured this)
                if (in_degree.find(neighbor) == in_degree.end()) {
                     in_degree[neighbor] = 0; // Should not happen if graph is consistent
                }
                in_degree[neighbor]++;
            }
        }

        // Initialize queue with nodes having in-degree 0
        for (const auto& pair : in_degree) {
            if (pair.second == 0) {
                queue.push_back(pair.first);
            }
        }

        size_t queue_idx = 0; // Current front of the queue

        while(queue_idx < queue.size()){
            NodeID u = queue[queue_idx++];
            sorted_order.push_back(u);

            // Since get_neighbors might throw if u somehow isn't in adj_list_
            // (e.g. if it was only ever a 'to' node and never a 'from' node with an explicit add_node call
            //  before becoming a 'to' node - though our add_edge prevents this)
            // we check. However, Kahn's algorithm implies u must be in adj_list_ to have outgoing edges.
            if (adj_list_.count(u)) {
                for (const NodeID& v : adj_list_.at(u)) {
                    if (in_degree.count(v)) { // Ensure v is in in_degree map
                        in_degree[v]--;
                        if (in_degree[v] == 0) {
                            queue.push_back(v);
                        }
                    }
                }
            }
        }

        if (sorted_order.size() != num_nodes()) {
            return aos_utils::make_unexpected("Graph has a cycle, topological sort not possible.");
        }

        return sorted_order;
    }

private:
    // Adjacency list: maps a node to a set of its neighbors.
    std::unordered_map<NodeID, std::unordered_set<NodeID>> adj_list_;

};

} // namespace cpp_utils
