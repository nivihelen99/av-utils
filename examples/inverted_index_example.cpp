#include "inverted_index.h"
#include <iostream>
#include <string>
#include <vector>

// Helper function to print a set of items
template<typename T>
void print_set(const T& set, const std::string& label) {
    std::cout << label << ": {";
    bool first = true;
    for (const auto& item : set) {
        if (!first) {
            std::cout << ", ";
        }
        std::cout << item;
        first = false;
    }
    std::cout << "}" << std::endl;
}

int main() {
    // Using strings for document IDs and tags for simplicity
    InvertedIndex<std::string, std::string> doc_tag_index;

    std::cout << "--- Initializing Document Tagging System ---" << std::endl;

    // Add some documents and their tags
    doc_tag_index.add("doc1", "c++");
    doc_tag_index.add("doc1", "programming");
    doc_tag_index.add("doc1", "performance");

    doc_tag_index.add("doc2", "python");
    doc_tag_index.add("doc2", "scripting");
    doc_tag_index.add("doc2", "data science");

    doc_tag_index.add("doc3", "c++");
    doc_tag_index.add("doc3", "game development");
    doc_tag_index.add("doc3", "graphics");

    doc_tag_index.add("doc4", "python");
    doc_tag_index.add("doc4", "web development");
    doc_tag_index.add("doc4", "programming"); // "programming" tag also in doc1

    std::cout << "\n--- Current State ---" << std::endl;
    if (doc_tag_index.empty()) {
        std::cout << "Index is empty." << std::endl;
    } else {
        std::cout << "Index has " << doc_tag_index.key_count() << " documents." << std::endl;
    }

    // Querying: What tags does doc1 have?
    std::cout << "\n--- Querying Tags for Documents ---" << std::endl;
    const auto& tags_for_doc1 = doc_tag_index.values_for("doc1");
    print_set(tags_for_doc1, "Tags for doc1");

    const auto& tags_for_doc2 = doc_tag_index.values_for("doc2");
    print_set(tags_for_doc2, "Tags for doc2");

    const auto& tags_for_doc_non_existent = doc_tag_index.values_for("doc_X");
    print_set(tags_for_doc_non_existent, "Tags for doc_X (non-existent)");


    // Querying: Which documents have the tag "c++"?
    std::cout << "\n--- Querying Documents for Tags ---" << std::endl;
    const auto& docs_with_cpp = doc_tag_index.keys_for("c++");
    print_set(docs_with_cpp, "Documents with tag 'c++'");

    const auto& docs_with_programming = doc_tag_index.keys_for("programming");
    print_set(docs_with_programming, "Documents with tag 'programming'");

    const auto& docs_with_java = doc_tag_index.keys_for("java"); // This tag was not added
    print_set(docs_with_java, "Documents with tag 'java' (non-existent tag)");

    // Checking for specific mappings
    std::cout << "\n--- Checking Specific Mappings ---" << std::endl;
    std::cout << "doc1 has tag 'performance': " << (doc_tag_index.contains("doc1", "performance") ? "Yes" : "No") << std::endl;
    std::cout << "doc2 has tag 'c++': " << (doc_tag_index.contains("doc2", "c++") ? "Yes" : "No") << std::endl;

    // Removing a specific tag from a document
    std::cout << "\n--- Modifying Mappings ---" << std::endl;
    std::cout << "Removing tag 'performance' from 'doc1'..." << std::endl;
    doc_tag_index.remove("doc1", "performance");
    print_set(doc_tag_index.values_for("doc1"), "Tags for doc1 (after removing 'performance')");
    print_set(doc_tag_index.keys_for("performance"), "Documents with tag 'performance' (after removing from doc1)");

    // Removing a document entirely
    std::cout << "\nRemoving 'doc2' entirely..." << std::endl;
    doc_tag_index.remove_key("doc2");
    std::cout << "doc2 has tag 'python': " << (doc_tag_index.contains("doc2", "python") ? "Yes" : "No") << std::endl;
    print_set(doc_tag_index.values_for("doc2"), "Tags for doc2 (after removing doc2)");
    print_set(doc_tag_index.keys_for("python"), "Documents with tag 'python' (after removing doc2)");
    print_set(doc_tag_index.keys_for("scripting"), "Documents with tag 'scripting' (after removing doc2)");


    // Removing a tag from all documents that have it
    std::cout << "\nRemoving tag 'programming' from all documents..." << std::endl;
    doc_tag_index.remove_value("programming");
    print_set(doc_tag_index.values_for("doc1"), "Tags for doc1 (after removing 'programming' globally)");
    print_set(doc_tag_index.values_for("doc4"), "Tags for doc4 (after removing 'programming' globally)");
    print_set(doc_tag_index.keys_for("programming"), "Documents with tag 'programming' (after global removal)");


    std::cout << "\n--- Final State ---" << std::endl;
    std::cout << "Iterating through remaining documents and their tags:" << std::endl;
    for (const auto& pair : doc_tag_index) { // Using the iterator
        print_set(pair.second, "Tags for " + pair.first);
    }

    if (doc_tag_index.empty()) {
        std::cout << "Index is now empty." << std::endl;
    } else {
        std::cout << "Index now has " << doc_tag_index.key_count() << " documents." << std::endl;
    }

    doc_tag_index.clear();
    std::cout << "\nCleared the index. Is it empty? " << (doc_tag_index.empty() ? "Yes" : "No") << std::endl;

    return 0;
}
