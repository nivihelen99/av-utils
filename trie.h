#pragma once
#include <iostream>
#include <string>
#include <vector>
// #include <unordered_map> // No longer needed
#include <memory>
// #include <array> // No longer directly used by TrieNode, Iterator will break.
#include <map>       // For std::map in TrieNode
#include <cctype>    // Added for std::tolower
#include <algorithm> // Added for std::transform
#include <fstream>   // For file operations
#include <sstream>   // For std::stringstream
#include <iterator>  // For std::forward_iterator_tag
#include <stack>     // For std::stack (Iterator will break)
#include <utility>   // For std::pair

class TrieNode {
public:
    // std::array<std::unique_ptr<TrieNode>, 256> children; // Old structure
    std::map<char, std::pair<std::string, std::unique_ptr<TrieNode>>> edges;
    bool isEndOfWord;
    int frequency; // Added for frequency counting
    
    TrieNode() : isEndOfWord(false), frequency(0) {}
};

class Trie {
private:
    std::unique_ptr<TrieNode> root;
    bool isCaseSensitive;
    Trie reversed_trie_;
    bool is_internally_reversed_instance_; // Flag to manage behavior for the reversed_trie_ member

    std::string normalizeString(const std::string& word) const {
        if (isCaseSensitive) {
            return word;
        }
        std::string lowerWord = word;
        std::transform(lowerWord.begin(), lowerWord.end(), lowerWord.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        return lowerWord;
    }

public:
    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = std::string;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const std::string*;
        using reference         = const std::string&;

    private:
        using EdgesMapConstIterator = typename std::map<char, std::pair<std::string, std::unique_ptr<TrieNode>>>::const_iterator;

        struct StackFrame {
            const TrieNode* node_ptr;
            EdgesMapConstIterator next_edge_it; // Iterator to the next edge to explore from node_ptr
            std::string label_of_edge_that_led_to_node_ptr; // For backtracking current_word_
        };

        const Trie* trie_ = nullptr;
        std::stack<StackFrame> node_stack_;
        std::string current_word_; // The word to be dereferenced

        void advance() {
            while (!node_stack_.empty()) {
                StackFrame& current_frame = node_stack_.top();
                const TrieNode* p_node = current_frame.node_ptr;

                if (current_frame.next_edge_it != p_node->edges.cend()) {
                    auto edge_to_process_it = current_frame.next_edge_it;
                    current_frame.next_edge_it++; // Advance iterator for this frame for next time

                    const std::string& edge_label = edge_to_process_it->second.first;
                    const TrieNode* child_node = edge_to_process_it->second.second.get();

                    current_word_ += edge_label;
                    node_stack_.push({child_node, child_node->edges.cbegin(), edge_label});

                    if (child_node->isEndOfWord) {
                        return; // Found a word
                    }
                    // Continue while loop to process the new top of stack (the child)
                } else {
                    // All children of p_node processed, backtrack
                    std::string label_that_led_to_this_p_node = current_frame.label_of_edge_that_led_to_node_ptr;
                    node_stack_.pop();

                    if (!label_that_led_to_this_p_node.empty()) {
                         if (current_word_.length() >= label_that_led_to_this_p_node.length() &&
                            current_word_.rfind(label_that_led_to_this_p_node) == current_word_.length() - label_that_led_to_this_p_node.length()) {
                           current_word_.erase(current_word_.length() - label_that_led_to_this_p_node.length());
                        } else {
                            // Error case or initial empty label for root's direct children
                        }
                    }
                    // Continue while loop to process new stack top (parent)
                }
            }
            // If stack becomes empty, iteration is complete.
            if (node_stack_.empty()) {
                current_word_.clear(); // Signify end
            }
        }

    public:
        Iterator(const Trie* trie_ptr, bool is_end_iterator) : trie_(trie_ptr) {
            current_word_ = ""; // End iterators and uninitialized iterators have empty word
            // For an end iterator, node_stack_ remains empty.
        }

        Iterator(const Trie* trie_ptr) : trie_(trie_ptr) {
            current_word_ = "";
            if (trie_ && trie_->root) {
                // For root, there's no edge that led to it, so label_of_edge_that_led_to_node_ptr is empty.
                node_stack_.push({trie_->root.get(), trie_->root.get()->edges.cbegin(), ""});
                if (trie_->root->isEndOfWord) {
                    // current_word_ is already "", which is correct for root being a word.
                    return;
                }
            }
            advance(); // Find the first actual word if root is not a word or trie is empty.
        }

        reference operator*() const { return current_word_; }
        pointer operator->() const { return &current_word_; }

        Iterator& operator++() {
            advance();
            return *this;
        }

        Iterator operator++(int) {
            Iterator temp = *this;
            ++(*this);
            return temp;
        }

        bool operator==(const Iterator& other) const {
            if (trie_ != other.trie_) return false;
            if (node_stack_.empty() && other.node_stack_.empty()) return true; // Both are end iterators
            if (node_stack_.empty() != other.node_stack_.empty()) return false; // One is end, other is not

            // Both are non-empty, compare current word and top node state (node_ptr and its next_edge_it)
            return current_word_ == other.current_word_ &&
                   node_stack_.top().node_ptr == other.node_stack_.top().node_ptr &&
                   node_stack_.top().next_edge_it == other.node_stack_.top().next_edge_it;
        }

        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }
    }; // End of Iterator class

    // Private constructor used for creating the reversed_trie_ member
    // The 'is_reversed_instance_tag' ensures that this reversed_trie_ instance
    // does not itself try to create another reversed_trie_.
    Trie(bool cs, bool is_reversed_instance_tag)
        : root(std::make_unique<TrieNode>()),
          isCaseSensitive(cs),
          // The reversed_trie_ member of an 'is_reversed_instance_tag=true' Trie is also marked as such.
          // This stops recursion effectively at one level. Its own reversed_trie_ will not be used.
          reversed_trie_(cs, true, true), // Pass dummy bool to select the deepest private constructor
          is_internally_reversed_instance_(is_reversed_instance_tag) {}

    // Deepest private constructor to break recursion for reversed_trie_'s own reversed_trie_
    // This constructor is only called by the one above for reversed_trie_.reversed_trie_
    Trie(bool cs, bool /*is_reversed_instance_tag*/, bool /*dummy_to_break_recursion*/)
        : root(std::make_unique<TrieNode>()),
          isCaseSensitive(cs),
          // reversed_trie_ member of this specific instance is default-constructed using the main public constructor,
          // which will then call the (cs, true) constructor for its own reversed_trie_, which then calls this.
          // This is still a bit tangled. The key is that is_internally_reversed_instance_ will be true.
          is_internally_reversed_instance_(true)
    {}

public:
    Trie(bool caseSensitive = true)
        : root(std::make_unique<TrieNode>()),
          isCaseSensitive(caseSensitive),
          reversed_trie_(caseSensitive, true), // Mark the member as an internal reversed instance
          is_internally_reversed_instance_(false) // This is a primary Trie instance
    {}
    
    Iterator begin() const { return Iterator(this); }
    Iterator end() const { return Iterator(this, true); }

    // Insert a word into the trie (Radix version)
    void insert(const std::string& word) {
        std::string normalizedWord = normalizeString(word);

        // Perform insertion on the main trie
        if (normalizedWord.empty()) {
            if (root) {
                root->isEndOfWord = true;
                root->frequency++;
            }
        } else {
            if (root) {
                 insertHelperRadix(root.get(), normalizedWord);
            }
        }

        if (!is_internally_reversed_instance_) {
            std::string reversed_normalized_word = normalizedWord;
            std::reverse(reversed_normalized_word.begin(), reversed_normalized_word.end());
            reversed_trie_.insert(reversed_normalized_word);
        }
    }

private: // Starting private section earlier for insertHelperRadix
    void insertHelperRadix(TrieNode* node, const std::string& current_word_part) {
        if (current_word_part.empty()) {
            node->isEndOfWord = true;
            node->frequency++;
            return;
        }

        char first_char = current_word_part[0];
        auto it = node->edges.find(first_char);

        if (it == node->edges.end()) { // No edge starts with this character
            auto new_child = std::make_unique<TrieNode>();
            new_child->isEndOfWord = true;
            new_child->frequency = 1; // New word ending
            node->edges[first_char] = {current_word_part, std::move(new_child)};
        } else { // Edge exists
            std::string& edge_label = it->second.first;
            std::unique_ptr<TrieNode>& child_node_ptr = it->second.second;

            size_t lcp_len = 0;
            while (lcp_len < edge_label.length() && lcp_len < current_word_part.length() &&
                   edge_label[lcp_len] == current_word_part[lcp_len]) {
                lcp_len++;
            }

            if (lcp_len == edge_label.length()) {
                // Edge label is fully matched, continue insertion into child node
                insertHelperRadix(child_node_ptr.get(), current_word_part.substr(lcp_len));
            } else if (lcp_len < edge_label.length()) {
                // Edge needs to be split
                std::string original_edge_suffix = edge_label.substr(lcp_len);
                std::string word_suffix = current_word_part.substr(lcp_len);

                auto intermediate_node = std::make_unique<TrieNode>();
                // The original child_node_ptr (and its original_edge_suffix) becomes a child of intermediate_node
                intermediate_node->edges[original_edge_suffix[0]] = {original_edge_suffix, std::move(child_node_ptr)};

                // Update the current edge to point to intermediate_node with LCP as label
                it->second.first = edge_label.substr(0, lcp_len); // LCP
                it->second.second = std::move(intermediate_node); // Points to new intermediate_node

                // If the word_suffix is empty, the intermediate_node itself is an end of a word.
                // Otherwise, continue inserting the word_suffix from the intermediate_node.
                insertHelperRadix(it->second.second.get(), word_suffix);
            } else { // lcp_len == current_word_part.length() AND lcp_len < edge_label.length()
                     // This means current_word_part is a prefix of edge_label. This case was covered by the one above.
                     // This specific 'else' means lcp_len == current_word_part.length() (because previous conditions were false)
                     // This implies the word being inserted is a prefix of the existing edge_label.

                // This case should be: current_word_part is fully matched (lcp_len == current_word_part.length())
                // but edge_label is longer (lcp_len < edge_label.length())
                // This is the "word is prefix of edge label" scenario.
                std::string new_edge_label_for_original_child = edge_label.substr(lcp_len); // = edge_label.substr(current_word_part.length())

                auto new_node_for_inserted_word = std::make_unique<TrieNode>();
                new_node_for_inserted_word->isEndOfWord = true;
                new_node_for_inserted_word->frequency = 1;

                new_node_for_inserted_word->edges[new_edge_label_for_original_child[0]] = {new_edge_label_for_original_child, std::move(child_node_ptr)};

                it->second.first = current_word_part; // Update edge label to the inserted word (which is the LCP)
                it->second.second = std::move(new_node_for_inserted_word);
            }
        }
    }

// public: // Public section resumes
    // Search for a word in the trie (Radix version)
    bool search(const std::string& word) const {
        std::string normalized_word = normalizeString(word);
        if (!root) return false; // Should not happen with current constructor
        if (normalized_word.empty()) {
            return root->isEndOfWord;
        }

        const TrieNode* current_node = root.get();
        std::string remaining_word = normalized_word;

        while (!remaining_word.empty() && current_node != nullptr) {
            char first_char = remaining_word[0];
            auto it = current_node->edges.find(first_char);

            if (it == current_node->edges.end()) {
                return false; // No edge starts with the needed character
            }

            const std::string& edge_label = it->second.first;
            const TrieNode* child_node = it->second.second.get();

            // Check if remaining_word starts with edge_label
            if (remaining_word.rfind(edge_label, 0) == 0) { // edge_label is a prefix of remaining_word
                if (edge_label.length() > remaining_word.length()) { // Should not happen if rfind is used correctly
                    return false; // Edge label is longer than remaining word but matches at pos 0. Defensive.
                }
                remaining_word = remaining_word.substr(edge_label.length());
                current_node = child_node;
            } else {
                return false; // Edge label is not a prefix of remaining_word
            }
        }
        return remaining_word.empty() && current_node && current_node->isEndOfWord;
    }
    
    // Check if any word starts with the given prefix (Radix version)
    bool startsWith(const std::string& prefix) const {
        std::string normalized_prefix = normalizeString(prefix);
        if (!root) return false; // Should not happen
        if (normalized_prefix.empty()) {
            return true; // Empty string is a prefix of any string
        }

        const TrieNode* current_node = root.get();
        std::string remaining_prefix = normalized_prefix;

        while (!remaining_prefix.empty() && current_node != nullptr) {
            char first_char = remaining_prefix[0];
            auto it = current_node->edges.find(first_char);

            if (it == current_node->edges.end()) {
                return false; // No edge starts with the needed character
            }

            const std::string& edge_label = it->second.first;
            const TrieNode* child_node = it->second.second.get();

            // Scenario 1: edge_label is a prefix of remaining_prefix
            // e.g., remaining_prefix = "applepie", edge_label = "apple"
            if (remaining_prefix.rfind(edge_label, 0) == 0) {
                remaining_prefix = remaining_prefix.substr(edge_label.length());
                current_node = child_node;
                // If remaining_prefix is now empty, loop terminates, returns true.
            }
            // Scenario 2: remaining_prefix is a prefix of edge_label
            // e.g., remaining_prefix = "app", edge_label = "apple"
            else if (edge_label.rfind(remaining_prefix, 0) == 0) {
                return true; // Entire original prefix is matched by the beginning of this edge_label
            }
            // Scenario 3: Mismatch
            else {
                return false; // Paths diverge
            }
        }
        // Loop finished. If remaining_prefix is empty, the entire prefix was matched.
        // If current_node is null, means we tried to go past a leaf in a way not matching the prefix.
        return remaining_prefix.empty();
    }
    
    // Get all words with a given prefix (Radix version)
    std::vector<std::string> getWordsWithPrefix(const std::string& prefix) const {
        std::string normalized_prefix = normalizeString(prefix);
        std::vector<std::string> result;
        if (!root) return result;

        if (normalized_prefix.empty()) {
            if (root->isEndOfWord) {
                result.push_back("");
            }
            collectWordsRadixHelper(root.get(), "", result);
            return result;
        }

        const TrieNode* current_node = root.get();
        std::string path_to_current_node = ""; // String accumulated by full edge matches to current_node
        std::string p_remaining = normalized_prefix;

        while (current_node != nullptr && !p_remaining.empty()) {
            char first_char = p_remaining[0];
            auto it = current_node->edges.find(first_char);

            if (it == current_node->edges.end()) {
                return result; // Prefix not found
            }

            const std::string& edge_label = it->second.first;
            const TrieNode* child_node = it->second.second.get();

            if (p_remaining.rfind(edge_label, 0) == 0) { // edge_label is prefix of p_remaining
                p_remaining = p_remaining.substr(edge_label.length());
                path_to_current_node += edge_label;
                current_node = child_node;
            } else if (edge_label.rfind(p_remaining, 0) == 0) { // p_remaining is prefix of edge_label
                // Prefix ends mid-edge. Collection starts from child_node.
                // The word formed by reaching child_node is path_to_current_node + edge_label.
                std::string base_word_for_collection = path_to_current_node + edge_label;
                if (child_node->isEndOfWord) {
                    result.push_back(base_word_for_collection);
                }
                collectWordsRadixHelper(child_node, base_word_for_collection, result);
                return result;
            } else { // Mismatch
                return result;
            }
        }

        // If loop finished, p_remaining should be empty if prefix was matched.
        if (p_remaining.empty() && current_node != nullptr) {
            // path_to_current_node is now equivalent to normalized_prefix
            if (current_node->isEndOfWord) {
                result.push_back(path_to_current_node);
            }
            collectWordsRadixHelper(current_node, path_to_current_node, result);
        }
        return result;
    }

    // Suffix search methods
    bool endsWith(const std::string& suffix) const {
        if (is_internally_reversed_instance_) {
            // This method should not be called on an internal reversed_trie directly.
            return false;
        }
        std::string normalized_suffix = normalizeString(suffix); // Use own normalizeString
        if (normalized_suffix.empty()) {
            return true; // Empty suffix matches any word
        }
        std::string reversed_suffix = normalized_suffix;
        std::reverse(reversed_suffix.begin(), reversed_suffix.end());
        // reversed_trie_ will use its own normalizeString if needed, but its case sensitivity
        // should match the main trie's.
        return reversed_trie_.startsWith(reversed_suffix);
    }

    std::vector<std::string> getWordsEndingWith(const std::string& suffix) const {
        if (is_internally_reversed_instance_) {
            return {};
        }
        std::string normalized_suffix = normalizeString(suffix); // Use own normalizeString
        std::string reversed_suffix = normalized_suffix;
        std::reverse(reversed_suffix.begin(), reversed_suffix.end());

        std::vector<std::string> reversed_matches = reversed_trie_.getWordsWithPrefix(reversed_suffix);
        std::vector<std::string> result;
        result.reserve(reversed_matches.size());

        for (const std::string& rev_word : reversed_matches) {
            std::string original_word = rev_word;
            std::reverse(original_word.begin(), original_word.end());
            result.push_back(original_word);
        }
        return result;
    }

    // Get the frequency of a word in the trie (NEEDS RADIX UPDATE)
    int getWordFrequency(const std::string& word) const {
        // This implementation is broken.
        return 0;
    }
    
    // Delete a word from the trie (Radix version)
    bool deleteWord(const std::string& word) {
        std::string normalized_word = normalizeString(word);
        bool main_deleted = false;
        if (!root) return false;

        if (normalized_word.empty()) {
            if (root->isEndOfWord) {
                root->isEndOfWord = false;
                root->frequency = 0;
                main_deleted = true;
            }
        } else {
            main_deleted = deleteRadixHelper(root.get(), normalized_word);
        }

        if (main_deleted && !is_internally_reversed_instance_) {
            std::string reversed_normalized_word = normalized_word;
            std::reverse(reversed_normalized_word.begin(), reversed_normalized_word.end());
            reversed_trie_.deleteWord(reversed_normalized_word);
        }
        return main_deleted;
    }
    
    // Print all words in the trie (for debugging)
    void printAllWords() const {
        std::vector<std::string> allWords;
        collectWords(root.get(), "", allWords);
        
        std::cout << "Words in trie: ";
        for (const auto& word : allWords) {
            std::cout << word << " ";
        }
        std::cout << std::endl;
    }

private:
    // Helper function to collect all words from a given node (Radix version)
    void collectWordsRadixHelper(const TrieNode* node, std::string current_word_base,
                                 std::vector<std::string>& result) const {
        if (node == nullptr) return;

        // Note: current_word_base is the word formed by reaching THIS node.
        // If this node itself is a word, it should have been added by the caller
        // before calling this helper, OR if it's the very first node from getWordsWithPrefix
        // where the prefix itself was a word.
        // This helper focuses on words *beyond* current_word_base, formed by its children.

        for (const auto& edge_entry : node->edges) {
            // char first_char_of_edge = edge_entry.first; // Key of map
            const std::string& edge_label = edge_entry.second.first;
            const TrieNode* child_node = edge_entry.second.second.get();

            if (child_node) {
                std::string word_via_this_edge = current_word_base + edge_label;
                if (child_node->isEndOfWord) {
                    result.push_back(word_via_this_edge);
                }
                collectWordsRadixHelper(child_node, word_via_this_edge, result);
            }
        }
    }

    // Old collectWords - now broken by TrieNode changes. Keep signature for now or remove.
    void collectWords(TrieNode* node, const std::string& currentWord, 
                     std::vector<std::string>& result) const {
        // This implementation is broken.
        // if (node->isEndOfWord) {
        //     result.push_back(currentWord);
        // }
        // for (const auto& edge_pair : node->edges) {
        //     // ...
        // }
    }

    // Helper function for deletion (Radix version)
    bool deleteRadixHelper(TrieNode* current_node, const std::string& remaining_word_part) {
        if (remaining_word_part.empty()) { // Base case: reached the node for the word
            if (current_node->isEndOfWord) {
                current_node->isEndOfWord = false;
                current_node->frequency = 0;
                return true; // Word was marked and now unmarked
            }
            return false; // Word was not actually marked at this node
        }

        char first_char = remaining_word_part[0];
        auto it_edge = current_node->edges.find(first_char);

        if (it_edge == current_node->edges.end()) {
            return false; // Word not found
        }

        std::string& edge_label = it_edge->second.first;
        std::unique_ptr<TrieNode>& child_node_ptr_ref = it_edge->second.second; // Reference to the unique_ptr in map

        if (remaining_word_part.rfind(edge_label, 0) == 0) { // edge_label is a prefix of remaining_word_part
            std::string next_remaining_word = remaining_word_part.substr(edge_label.length());

            if (deleteRadixHelper(child_node_ptr_ref.get(), next_remaining_word)) {
                // Child or descendant was modified/deleted, check child_node_ptr_ref
                TrieNode* child_node = child_node_ptr_ref.get(); // For inspection

                // Condition 1: Child is now a non-word leaf node -> remove edge to child
                if (!child_node->isEndOfWord && child_node->edges.empty()) {
                    current_node->edges.erase(it_edge); // Erases based on iterator, child_node_ptr_ref (unique_ptr) is destroyed
                    return true;
                }

                // Condition 2: Child is non-word and has only one child -> merge
                if (!child_node->isEndOfWord && child_node->edges.size() == 1) {
                    auto& grandchild_edge_entry = child_node->edges.begin(); // Get the single edge from child
                    std::string& grandchild_label = grandchild_edge_entry->second.first;
                    std::unique_ptr<TrieNode>& grandchild_node_unique_ptr = grandchild_edge_entry->second.second;

                    // Update current edge (it_edge points to the edge from current_node to child_node)
                    edge_label += grandchild_label; // Append grandchild's edge label to current edge's label
                    child_node_ptr_ref = std::move(grandchild_node_unique_ptr); // Current edge now points to grandchild
                                                                                // child_node is bypassed and will be deleted
                    return true;
                }
                return true; // Modification happened below, but child itself is not merged up/removed by current_node
            }
            return false; // Recursive call returned false, no changes below
        } else {
            // remaining_word_part is not prefixed by edge_label (e.g. word="applepie", edge="apply")
            // or edge_label is not a prefix of remaining_word_part (this is covered by the rfind check)
            // Essentially, if rfind fails, the path diverges.
            return false; // Word not found
        }
    }

    // Old deleteHelper - now broken and replaced by deleteRadixHelper
    bool deleteHelper(TrieNode* node, const std::string& word, int index) {
        // This implementation is broken.
        return false;
    }

    // New Radix-compatible helper for collecting words with frequencies
    void collectWordsWithFrequenciesRadixHelper(const TrieNode* node, std::string current_word_base,
                                                std::vector<std::pair<std::string, int>>& allWordsWithFreq) const {
        if (node == nullptr) return;

        // current_word_base is the word formed by path to THIS node.
        // Iterate through its outgoing edges.
        for (const auto& edge_entry : node->edges) {
            const std::string& edge_label = edge_entry.second.first;
            const TrieNode* child_node = edge_entry.second.second.get();

            if (child_node) {
                std::string word_via_this_edge = current_word_base + edge_label;
                if (child_node->isEndOfWord) {
                    allWordsWithFreq.push_back({word_via_this_edge, child_node->frequency});
                }
                collectWordsWithFrequenciesRadixHelper(child_node, word_via_this_edge, allWordsWithFreq);
            }
        }
    }

    // Old collectWordsWithFrequencies - broken by TrieNode changes
    void collectWordsWithFrequencies(const TrieNode* node, std::string currentWord,
                                     std::vector<std::pair<std::string, int>>& allWordsWithFreq) const {
        // This implementation is broken.
    }

    // Radix-compatible wildcard search helpers
    void matchEdgeAgainstPattern(const std::string& edge_label, size_t edge_idx,
                                 const TrieNode* edge_target_node,
                                 const std::string& pattern, size_t pattern_idx,
                                 std::string current_word_base,
                                 std::vector<std::string>& result) const {
        if (edge_idx == edge_label.length()) { // Successfully matched the entire edge_label
            wildcardSearchRecursive(edge_target_node, pattern, pattern_idx, current_word_base + edge_label, result);
            return;
        }

        if (pattern_idx == pattern.length()) { // Pattern ended mid-edge
            return;
        }

        // Current pattern char must not be '*' here, as wildcardSearchRecursive handles '*' globally.
        if (pattern[pattern_idx] == '?' || pattern[pattern_idx] == edge_label[edge_idx]) {
            matchEdgeAgainstPattern(edge_label, edge_idx + 1, edge_target_node, pattern, pattern_idx + 1, current_word_base, result);
        }
        // Else: mismatch, just return
    }

    void wildcardSearchRecursive(const TrieNode* node, const std::string& pattern,
                                 size_t pattern_idx, std::string current_word,
                                 std::vector<std::string>& result) const {
        if (node == nullptr) return;

        if (pattern_idx == pattern.length()) { // Pattern exhausted
            if (node->isEndOfWord) {
                result.push_back(current_word);
            }
            return;
        }

        char p_char = pattern[pattern_idx];

        if (p_char == '*') {
            // Option 1: '*' matches empty string (skip '*' and try to match rest of pattern from current node)
            wildcardSearchRecursive(node, pattern, pattern_idx + 1, current_word, result);

            // Option 2: '*' consumes one or more characters by consuming an entire edge
            // The '*' stays at pattern_idx, meaning it can consume more edges from the child.
            for (const auto& edge_entry : node->edges) {
                const std::string& edge_label = edge_entry.second.first;
                const TrieNode* child_node = edge_entry.second.second.get();
                if (child_node) { // Should always be true for valid entries
                     // '*' attempts to consume this edge_label
                    wildcardSearchRecursive(child_node, pattern, pattern_idx, current_word + edge_label, result);
                }
            }
        } else { // Character is '?' or a literal
            for (const auto& edge_entry : node->edges) {
                const std::string& edge_label = edge_entry.second.first;
                const TrieNode* child_node = edge_entry.second.second.get();
                if (child_node) { // Should always be true
                    // Try to match this edge_label against the pattern starting from p_char
                    matchEdgeAgainstPattern(edge_label, 0, child_node, pattern, pattern_idx, current_word, result);
                }
            }
        }
    }

    // Old wildcardSearchHelper - broken by TrieNode changes.
    void wildcardSearchHelper(const TrieNode* node, const std::string& pattern, size_t patternIndex,
                              std::string currentWordBuilt, std::vector<std::string>& result) const {
        // This implementation is broken.
    }

    // Helper for fuzzy search: calculates next DP row for Levenshtein distance
    std::vector<int> calculateNextDpRow(const std::vector<int>& prev_dp_row,
                                        char current_char_from_trie_path,
                                        const std::string& query_word) const {
        std::vector<int> new_dp_row(query_word.length() + 1);
        new_dp_row[0] = prev_dp_row[0] + 1; // Cost of deleting current_char_from_trie_path

        for (size_t j = 1; j <= query_word.length(); ++j) {
            int cost = (current_char_from_trie_path == query_word[j-1]) ? 0 : 1; // Substitution cost
            new_dp_row[j] = std::min({
                new_dp_row[j-1] + 1,          // Deletion from query (insertion into trie path)
                prev_dp_row[j] + 1,           // Insertion into query (deletion from trie path char)
                prev_dp_row[j-1] + cost       // Substitution or match
            });
        }
        return new_dp_row;
    }

    // Recursive helper for fuzzy search
    void fuzzySearchRadixRecursive(const TrieNode* node,
                                   std::vector<int> current_dp_row, // Passed by value as it's modified per path
                                   const std::string& query,
                                   int max_k,
                                   std::string current_word_formed,
                                   std::map<std::string, int>& results_map) const {
        if (node == nullptr) return;

        // Check if current word is a candidate
        if (node->isEndOfWord) {
            int distance = current_dp_row.back(); // Distance to full query word
            if (distance <= max_k) {
                auto it = results_map.find(current_word_formed);
                if (it == results_map.end() || distance < it->second) {
                    results_map[current_word_formed] = distance;
                }
            }
        }

        // Pruning: if min cost in current DP row already exceeds max_k, no path from here can be better
        if (!current_dp_row.empty() && *std::min_element(current_dp_row.begin(), current_dp_row.end()) > max_k) {
            return;
        }

        // Iterate through edges
        for (const auto& edge_entry : node->edges) {
            const std::string& edge_label = edge_entry.second.first;
            const TrieNode* child_node = edge_entry.second.second.get();
            if (!child_node) continue;

            std::vector<int> dp_row_for_child = current_dp_row;
            std::string temp_word_along_edge = current_word_formed;

            for (char ch_in_label : edge_label) {
                dp_row_for_child = calculateNextDpRow(dp_row_for_child, ch_in_label, query);
                // temp_word_along_edge += ch_in_label; // this string is built before recursive call
            }
            // After processing all chars in edge_label, dp_row_for_child is the DP row for (current_word_formed + edge_label)
            fuzzySearchRadixRecursive(child_node, dp_row_for_child, query, max_k, current_word_formed + edge_label, results_map);
        }
    }


// public: // Public section resumes after private helpers
    // ... (other public methods like Trie constructor, search, etc. remain here)
    std::vector<std::string> wildcardSearch(const std::string& pattern) const {
        std::string normalized_pattern = normalizeString(pattern);
        std::vector<std::string> result;
        if (!root) return result;

        wildcardSearchRecursive(root.get(), normalized_pattern, 0, "", result);

        return result;
    }

    std::vector<std::pair<std::string, int>> fuzzySearch(const std::string& query_word, int max_k) const {
        std::string normalized_query = normalizeString(query_word);
        std::map<std::string, int> results_map;
        if (!root) return {};

        std::vector<int> initial_dp_row(normalized_query.length() + 1);
        for (size_t i = 0; i <= normalized_query.length(); ++i) {
            initial_dp_row[i] = static_cast<int>(i); // Cost of matching query prefix with empty string
        }

        // Initial call for root node. The "word formed" at root is empty string.
        // If root itself is a word and query is empty, this needs to be handled.
        // Current fuzzySearchRadixRecursive handles node->isEndOfWord with current_dp_row.
        // If query is empty, initial_dp_row = [0], distance = 0.
        // If root is word, "" is added if distance (0) <= max_k. Correct.

        fuzzySearchRadixRecursive(root.get(), initial_dp_row, normalized_query, max_k, "", results_map);

        std::vector<std::pair<std::string, int>> final_results;
        for (const auto& pair : results_map) {
            final_results.push_back(pair);
        }
        // Optional: Sort results, e.g., by distance then alphabetically
        std::sort(final_results.begin(), final_results.end(),
                  [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
            if (a.second != b.second) {
                return a.second < b.second;
            }
            return a.first < b.first;
        });
        return final_results;
    }

    bool saveToFile(const std::string& filename) const {
        std::vector<std::pair<std::string, int>> allWordsWithFreq;
        if (!root) return false; // Should not happen

        // Handle empty string case if it's a word
        if (root->isEndOfWord) {
            allWordsWithFreq.push_back({"", root->frequency});
        }
        collectWordsWithFrequenciesRadixHelper(root.get(), "", allWordsWithFreq);

        std::ofstream output_file(filename);
        if (!output_file.is_open()) {
            std::cerr << "Error: Could not open file for saving: " << filename << std::endl;
            return false;
        }

        for (const auto& pair : allWordsWithFreq) {
            output_file << pair.first << " " << pair.second << std::endl;
        }

        output_file.close();
        return true;
    }

    bool loadFromFile(const std::string& filename) {
        root = std::make_unique<TrieNode>();
        if (!is_internally_reversed_instance_) {
            // For a primary Trie, reset its reversed_trie_ properly.
            reversed_trie_ = Trie(this->isCaseSensitive, true);
        }
        // If this IS an internal reversed instance, its own reversed_trie_ member is not used/reset further.

        if (!root) return false;

        std::ifstream input_file(filename);
        if (!input_file.is_open()) {
            std::cerr << "Error: Could not open file for loading: " << filename << std::endl;
            return false;
        }

        std::string line;
        while (std::getline(input_file, line)) {
            std::stringstream ss(line);
            std::string word_from_file; // Renamed to avoid confusion with normalized_word
            int freq_from_file;

            if (ss >> word_from_file >> freq_from_file) {
                this->insert(word_from_file);

                std::string normalized_word_nav = normalizeString(word_from_file);
                TrieNode* target_node = root.get();

                if (normalized_word_nav.empty()) {
                    if (target_node->isEndOfWord) { // target_node is root here
                        target_node->frequency = freq_from_file;
                    }
                } else {
                    std::string path_part_remaining = normalized_word_nav;
                    bool node_found_for_freq_set = true;

                    while(!path_part_remaining.empty() && target_node) {
                        char first_char_nav = path_part_remaining[0];
                        auto it_nav = target_node->edges.find(first_char_nav);

                        if (it_nav == target_node->edges.end()) {
                            node_found_for_freq_set = false;
                            break;
                        }

                        const std::string& current_edge_label = it_nav->second.first;
                        if (path_part_remaining.rfind(current_edge_label, 0) == 0) { // current_edge_label is prefix
                            path_part_remaining = path_part_remaining.substr(current_edge_label.length());
                            target_node = it_nav->second.second.get();
                        } else {
                            node_found_for_freq_set = false;
                            break;
                        }
                    }

                    if (node_found_for_freq_set && path_part_remaining.empty() && target_node && target_node->isEndOfWord) {
                        target_node->frequency = freq_from_file;
                    } else {
                        // std::cerr << "Warning: Could not accurately set frequency for word '" << word_from_file << "' from file." << std::endl;
                    }
                }
            }
        }
        input_file.close();
        return true;
    }
};

