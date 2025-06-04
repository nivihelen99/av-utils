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
        const Trie* trie_ = nullptr;
        std::stack<std::pair<const TrieNode*, int>> node_stack_; // {Node ptr, next_child_idx_to_visit}
        std::string current_word_;

        void advance() {
            while (!node_stack_.empty()) {
                const TrieNode* p_node = node_stack_.top().first;
                int& next_child_idx_ref = node_stack_.top().second;

                bool descended = false;
                for (int i = next_child_idx_ref; i < 256; ++i) {
                    next_child_idx_ref = i + 1;
                    const TrieNode* child = p_node->children[i].get();
                    if (child) {
                        current_word_ += static_cast<char>(i);
                        node_stack_.push({child, 0});
                        if (child->isEndOfWord) {
                            return;
                        }
                        descended = true;
                        break;
                    }
                }

                if (descended) {
                    continue;
                }

                node_stack_.pop();
                if (!current_word_.empty()) {
                    current_word_.pop_back();
                }
            }
        }

    public:
        Iterator(const Trie* trie_ptr, bool is_end_iterator) : trie_(trie_ptr) {
            // If is_end_iterator, stack remains empty, current_word_ empty.
            // This constructor is primarily for creating end() iterators.
            // No explicit action needed if is_end_iterator is true and default member initializers are sufficient.
        }

        Iterator(const Trie* trie_ptr) : trie_(trie_ptr) {
            if (trie_ && trie_->root) {
                node_stack_.push({trie_->root.get(), 0});
                current_word_ = ""; // Should be empty for root
                if (trie_->root->isEndOfWord) { // Check if empty string is a word
                    return;
                }
            }
            // If trie is empty or root is not a word, find the first word
            advance();
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
            // End iterators are equal if both stacks are empty and they point to the same trie
            if (node_stack_.empty() && other.node_stack_.empty()) {
                return trie_ == other.trie_;
            }
            // If one stack is empty and the other is not, they are not equal
            if (node_stack_.empty() != other.node_stack_.empty()) {
                return false;
            }
            // Both stacks are non-empty: compare trie, current word, and top of stack node
            return trie_ == other.trie_ &&
                   current_word_ == other.current_word_ &&
                   node_stack_.top().first == other.node_stack_.top().first;
        }

        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }
    }; // End of Iterator class

    Trie(bool caseSensitive = true) : root(std::make_unique<TrieNode>()), isCaseSensitive(caseSensitive) {}
    
    Iterator begin() const { return Iterator(this); }
    Iterator end() const { return Iterator(this, true); }

    // Insert a word into the trie (Radix version)
    void insert(const std::string& word) {
        std::string normalizedWord = normalizeString(word);
        if (normalizedWord.empty() && root) { // Handle empty string insertion specifically at root
            root->isEndOfWord = true;
            root->frequency++;
            return;
        }
        if (root) { // Ensure root exists
             insertHelperRadix(root.get(), normalizedWord);
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
    
    // Check if any word starts with the given prefix
    bool startsWith(const std::string& prefix) const {
        std::string normalizedPrefix = normalizeString(prefix);
        TrieNode* current = root.get();
        
        for (char ch : normalizedPrefix) {
            unsigned char u_ch = static_cast<unsigned char>(ch);
            if (!current->children[u_ch]) {
                return false;
            }
            current = current->children[u_ch].get();
        }
        return true;
    }
    
    // Get all words with a given prefix
    std::vector<std::string> getWordsWithPrefix(const std::string& prefix) const {
        std::string normalizedPrefix = normalizeString(prefix);
        std::vector<std::string> result;
        TrieNode* current = root.get();
        
        // Navigate to the prefix node
        for (char ch : normalizedPrefix) {
            unsigned char u_ch = static_cast<unsigned char>(ch);
            if (!current->children[u_ch]) {
                return result; // Empty vector if prefix doesn't exist
            }
            current = current->children[u_ch].get();
        }
        
        // DFS to collect all words from this node
        collectWords(current, normalizedPrefix, result);
        return result;
    }

    // Get the frequency of a word in the trie
    int getWordFrequency(const std::string& word) const {
        std::string normalizedWord = normalizeString(word);
        TrieNode* current = root.get();

        for (char ch : normalizedWord) {
            unsigned char u_ch = static_cast<unsigned char>(ch);
            if (!current->children[u_ch]) {
                return 0; // Word not found
            }
            current = current->children[u_ch].get();
        }

        if (current->isEndOfWord) {
            return current->frequency;
        }
        return 0; // Word found, but not marked as end of a word
    }
    
    // Delete a word from the trie
    bool deleteWord(const std::string& word) {
        std::string normalizedWord = normalizeString(word);
        return deleteHelper(root.get(), normalizedWord, 0);
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
    // Helper function to collect all words from a given node (NEEDS RADIX UPDATE)
    void collectWords(TrieNode* node, const std::string& currentWord, 
                     std::vector<std::string>& result) const {
        // This implementation is now broken due to TrieNode changes.
        // Needs to iterate over node->edges map.
        if (node->isEndOfWord) {
            result.push_back(currentWord);
        }
        
        // for (int i = 0; i < 256; ++i) { // Old logic
        //     if (node->children[i]) {
        //         collectWords(node->children[i].get(), currentWord + static_cast<char>(i), result);
        //     }
        // }
        for (const auto& edge_pair : node->edges) {
            // const std::string& edge_label = edge_pair.second.first;
            // const auto& child_node = edge_pair.second.second;
            // collectWords(child_node.get(), currentWord + edge_label, result); // Simplified, needs correct prefix
        }
    }
    
    // Helper function for deletion (NEEDS RADIX UPDATE)
    bool deleteHelper(TrieNode* node, const std::string& word, int index) {
        // This implementation is now broken.
        return false;
    }

    void collectWordsWithFrequencies(const TrieNode* node, std::string currentWord,
                                     std::vector<std::pair<std::string, int>>& allWordsWithFreq) const {
        // This implementation is now broken due to TrieNode changes.
        if (node == nullptr) {
            return;
        }
        if (node->isEndOfWord) {
            allWordsWithFreq.push_back({currentWord, node->frequency});
        }
        
        // for (int i = 0; i < 256; ++i) { // Old logic
        //     if (node->children[i]) {
        //         collectWordsWithFrequencies(node->children[i].get(), currentWord + static_cast<char>(i), allWordsWithFreq);
        //     }
        // }
    }

    void wildcardSearchHelper(const TrieNode* node, const std::string& pattern, size_t patternIndex,
                              std::string currentWord, std::vector<std::string>& result) const {
        // This implementation is now broken due to TrieNode changes.
        if (node == nullptr) {
            return;
        }
        // ... rest of logic is broken
    }

// public: // Public section resumes after private helpers
    // ... (other public methods like Trie constructor, search, etc. remain here)
    std::vector<std::string> wildcardSearch(const std::string& pattern) const {
        std::string normalized_pattern = normalizeString(pattern);
        std::vector<std::string> result;
        // Optimization: if isCaseSensitive is false, the pattern is already lowercase.
        // However, collectWords returns original casing if stored (which it isn't here, words are built char by char)
        // The current collectWords builds words using chars from iteration, which are fine.
        // For wildcard, currentWord is built using chars from pattern (if not '?') or iteration.
        // If the trie is case-insensitive, all chars inserted are already lowercase.
        // So, currentWord will be lowercase. This is consistent.

        wildcardSearchHelper(root.get(), normalized_pattern, 0, "", result);
        
        // To remove duplicates that might arise from complex '*' patterns, e.g. "a**b"
        // std::sort(result.begin(), result.end());
        // result.erase(std::unique(result.begin(), result.end()), result.end());
        // For now, let's skip duplicate removal to see if the core logic is fine.
        // The provided recursive structure for '*' (match zero then match one or more) is designed to avoid duplicates
        // that plague simpler "match one char and recurse on *" or "skip *" approaches.

        return result;
    }

    bool saveToFile(const std::string& filename) const {
        std::vector<std::pair<std::string, int>> allWordsWithFreq;
        collectWordsWithFrequencies(root.get(), "", allWordsWithFreq);

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
        root = std::make_unique<TrieNode>(); // Clear current Trie data

        std::ifstream input_file(filename);
        if (!input_file.is_open()) {
            std::cerr << "Error: Could not open file for loading: " << filename << std::endl;
            return false;
        }

        std::string line;
        while (std::getline(input_file, line)) {
            std::stringstream ss(line);
            std::string word;
            int freq;

            if (ss >> word >> freq) { // Check if parsing is successful
                // Insert the word. This handles normalization and sets isEndOfWord.
                // It also increments frequency, but we'll override it next.
                this->insert(word);

                // Now, navigate to the node and set the correct frequency.
                std::string normalized_word_for_lookup = normalizeString(word);
                TrieNode* current = root.get();
                bool path_exists = true;
                for (char ch_lookup : normalized_word_for_lookup) {
                    unsigned char u_ch_lookup = static_cast<unsigned char>(ch_lookup);
                    if (!current->children[u_ch_lookup]) {
                        // This should ideally not happen if insert() worked correctly.
                        // Or file is malformed for a word that insert() could not process.
                        path_exists = false;
                        break;
                    }
                    current = current->children[u_ch_lookup].get();
                }

                if (path_exists && current != nullptr && current->isEndOfWord) {
                    current->frequency = freq;
                } else {
                    // Optional: Log an error if a word from file couldn't be properly processed after insert
                    // std::cerr << "Warning: Could not set frequency for word '" << word << "' from file." << std::endl;
                }
            }
        }

        input_file.close();
        return true;
    }
};

