#pragma once

// #include <nlohmann/json.hpp>
#include "json.hpp"
#include <string>
#include <vector>
#include <stdexcept>

using json = nlohmann::json;

/**
 * Exception thrown when patch operations fail
 */
class JsonPatchException : public std::runtime_error {
public:
    explicit JsonPatchException(const std::string& message) : std::runtime_error(message) {}
};

/**
 * Represents a single JSON patch operation following RFC 6902
 */
struct JsonPatchOperation {
    enum class Type {
        ADD,
        REMOVE,
        REPLACE,
        MOVE,
        COPY,
        TEST
    };
    
    Type op;
    std::string path;
    std::string from;  // Used for move/copy operations
    json value;        // Used for add/replace/test operations
    
    JsonPatchOperation(Type op_type, const std::string& path_str)
        : op(op_type), path(path_str) {}
    
    JsonPatchOperation(Type op_type, const std::string& path_str, const json& val)
        : op(op_type), path(path_str), value(val) {}
    
    JsonPatchOperation(Type op_type, const std::string& from_path, const std::string& to_path)
        : op(op_type), path(to_path), from(from_path) {}
    
    // Convert operation to JSON representation
    json to_json() const;
    
    // Create operation from JSON representation
    static JsonPatchOperation from_json(const json& j);
    
private:
    static std::string type_to_string(Type t);
    static Type string_to_type(const std::string& s);
};

/**
 * Configuration options for diff generation
 */
struct JsonDiffOptions {
    bool detect_moves = false;          // Detect move operations in arrays
    bool use_test_operations = false;   // Include test operations for validation
    bool compact_patches = true;        // Merge redundant operations
    
    JsonDiffOptions() = default;
};

/**
 * Main JsonPatch class for generating and applying JSON patches
 */
class JsonPatch {
private:
    std::vector<JsonPatchOperation> operations_;
    
public:
    JsonPatch() = default;
    explicit JsonPatch(std::vector<JsonPatchOperation> ops) : operations_(std::move(ops)) {}
    
    // Generate patch to transform 'from' into 'to'
    static JsonPatch diff(const json& from, const json& to, const JsonDiffOptions& options = JsonDiffOptions{});
    
    // Apply this patch to the given JSON document
    json apply(const json& document) const;
    
    // Apply patch without modifying the original (dry run validation)
    bool dry_run(const json& document) const;
    
    // Convert patch to JSON array representation
    json to_json() const;
    
    // Create patch from JSON array representation
    static JsonPatch from_json(const json& j);
    
    // Generate inverse patch for undo operations
    JsonPatch invert(const json& original_document) const;
    
    // Check if patch has conflicts when applied to document
    bool has_conflict(const json& document) const;
    
    // Compact redundant operations
    JsonPatch compact() const;
    
    // Get operations count
    size_t size() const { return operations_.size(); }
    
    // Check if patch is empty
    bool empty() const { return operations_.empty(); }
    
    // Iterator access
    auto begin() const { return operations_.begin(); }
    auto end() const { return operations_.end(); }
    
private:
    // Helper methods for diff generation
    static void generate_diff_recursive(const json& from, const json& to, 
                                      const std::string& base_path,
                                      std::vector<JsonPatchOperation>& operations,
                                      const JsonDiffOptions& options);
    
    static void handle_object_diff(const json& from, const json& to,
                                 const std::string& base_path,
                                 std::vector<JsonPatchOperation>& operations,
                                 const JsonDiffOptions& options);
    
    static void handle_array_diff(const json& from, const json& to,
                                const std::string& base_path,
                                std::vector<JsonPatchOperation>& operations,
                                const JsonDiffOptions& options);
    
    // Helper methods for patch application
    json apply_operation(const json& document, const JsonPatchOperation& op) const;
    json get_value_at_path(const json& document, const std::string& path) const;
    json set_value_at_path(const json& document, const std::string& path, const json& value) const;
    json remove_value_at_path(const json& document, const std::string& path) const;
    
    // Path manipulation utilities
    static std::vector<std::string> split_path(const std::string& path);
    static std::string join_path(const std::vector<std::string>& components);
    static std::string escape_path_component(const std::string& component);
    static std::string unescape_path_component(const std::string& component);
};

// Convenience function for quick diff generation
JsonPatch diff(const json& from, const json& to, const JsonDiffOptions& options = JsonDiffOptions{});


#include <algorithm>
#include <sstream>
#include <regex>

// JsonPatchOperation implementation

json JsonPatchOperation::to_json() const {
    json result;
    result["op"] = type_to_string(op);
    result["path"] = path;
    
    switch (op) {
        case Type::ADD:
        case Type::REPLACE:
        case Type::TEST:
            result["value"] = value;
            break;
        case Type::MOVE:
        case Type::COPY:
            result["from"] = from;
            break;
        case Type::REMOVE:
            // No additional fields needed
            break;
    }
    
    return result;
}

JsonPatchOperation JsonPatchOperation::from_json(const json& j) {
    if (!j.contains("op") || !j.contains("path")) {
        throw JsonPatchException("Invalid patch operation: missing 'op' or 'path'");
    }
    
    std::string op_str = j["op"];
    std::string path_str = j["path"];
    Type op_type = string_to_type(op_str);
    
    switch (op_type) {
        case Type::ADD:
        case Type::REPLACE:
        case Type::TEST:
            if (!j.contains("value")) {
                throw JsonPatchException("Missing 'value' field for " + op_str + " operation");
            }
            return JsonPatchOperation(op_type, path_str, j["value"]);
            
        case Type::MOVE:
        case Type::COPY:
            if (!j.contains("from")) {
                throw JsonPatchException("Missing 'from' field for " + op_str + " operation");
            }
            return JsonPatchOperation(op_type, j["from"], path_str);
            
        case Type::REMOVE:
            return JsonPatchOperation(op_type, path_str);
    }
    
    throw JsonPatchException("Unknown operation type: " + op_str);
}

std::string JsonPatchOperation::type_to_string(Type t) {
    switch (t) {
        case Type::ADD: return "add";
        case Type::REMOVE: return "remove";
        case Type::REPLACE: return "replace";
        case Type::MOVE: return "move";
        case Type::COPY: return "copy";
        case Type::TEST: return "test";
    }
    return "unknown";
}

JsonPatchOperation::Type JsonPatchOperation::string_to_type(const std::string& s) {
    if (s == "add") return Type::ADD;
    if (s == "remove") return Type::REMOVE;
    if (s == "replace") return Type::REPLACE;
    if (s == "move") return Type::MOVE;
    if (s == "copy") return Type::COPY;
    if (s == "test") return Type::TEST;
    throw JsonPatchException("Unknown operation type: " + s);
}

// JsonPatch implementation

JsonPatch JsonPatch::diff(const json& from, const json& to, const JsonDiffOptions& options) {
    std::vector<JsonPatchOperation> operations;
    generate_diff_recursive(from, to, "", operations, options);
    
    JsonPatch patch(std::move(operations));
    return options.compact_patches ? patch.compact() : patch;
}

void JsonPatch::generate_diff_recursive(const json& from, const json& to, 
                                       const std::string& base_path,
                                       std::vector<JsonPatchOperation>& operations,
                                       const JsonDiffOptions& options) {
    if (from == to) {
        return; // No difference
    }
    
    // Handle type changes
    if (from.type() != to.type()) {
        operations.emplace_back(JsonPatchOperation::Type::REPLACE, base_path, to);
        return;
    }
    
    switch (from.type()) {
        case json::value_t::object:
            handle_object_diff(from, to, base_path, operations, options);
            break;
        case json::value_t::array:
            handle_array_diff(from, to, base_path, operations, options);
            break;
        default:
            // Primitive values that are different
            operations.emplace_back(JsonPatchOperation::Type::REPLACE, base_path, to);
            break;
    }
}

void JsonPatch::handle_object_diff(const json& from, const json& to,
                                 const std::string& base_path,
                                 std::vector<JsonPatchOperation>& operations,
                                 const JsonDiffOptions& options) {
    // Handle removed keys
    for (auto it = from.begin(); it != from.end(); ++it) {
        if (to.find(it.key()) == to.end()) {
            std::string key_path = base_path + "/" + escape_path_component(it.key());
            operations.emplace_back(JsonPatchOperation::Type::REMOVE, key_path);
        }
    }
    
    // Handle added and modified keys
    for (auto it = to.begin(); it != to.end(); ++it) {
        std::string key_path = base_path + "/" + escape_path_component(it.key());
        
        auto from_it = from.find(it.key());
        if (from_it == from.end()) {
            // Key is new
            operations.emplace_back(JsonPatchOperation::Type::ADD, key_path, it.value());
        } else {
            // Key exists, check for changes recursively
            generate_diff_recursive(from_it.value(), it.value(), key_path, operations, options);
        }
    }
}

void JsonPatch::handle_array_diff(const json& from, const json& to,
                                const std::string& base_path,
                                std::vector<JsonPatchOperation>& operations,
                                const JsonDiffOptions& options) {
    size_t from_size = from.size();
    size_t to_size = to.size();
    size_t min_size = std::min(from_size, to_size);
    
    // Check existing elements for changes
    for (size_t i = 0; i < min_size; ++i) {
        std::string index_path = base_path + "/" + std::to_string(i);
        generate_diff_recursive(from[i], to[i], index_path, operations, options);
    }
    
    // Handle size differences
    if (to_size > from_size) {
        // Elements added
        for (size_t i = from_size; i < to_size; ++i) {
            std::string index_path = base_path + "/" + std::to_string(i);
            operations.emplace_back(JsonPatchOperation::Type::ADD, index_path, to[i]);
        }
    } else if (from_size > to_size) {
        // Elements removed (remove from end to avoid index shifting issues)
        for (size_t i = from_size; i > to_size; --i) {
            std::string index_path = base_path + "/" + std::to_string(i - 1);
            operations.emplace_back(JsonPatchOperation::Type::REMOVE, index_path);
        }
    }
}

json JsonPatch::apply(const json& document) const {
    json result = document;
    
    for (const auto& op : operations_) {
        result = apply_operation(result, op);
    }
    
    return result;
}

json JsonPatch::apply_operation(const json& document, const JsonPatchOperation& op) const {
    switch (op.op) {
        case JsonPatchOperation::Type::ADD:
            return set_value_at_path(document, op.path, op.value);
            
        case JsonPatchOperation::Type::REMOVE:
            return remove_value_at_path(document, op.path);
            
        case JsonPatchOperation::Type::REPLACE: {
            // For replace, we need to ensure the path exists
            try {
                get_value_at_path(document, op.path);
                return set_value_at_path(document, op.path, op.value);
            } catch (const JsonPatchException&) {
                throw JsonPatchException("Cannot replace at non-existent path: " + op.path);
            }
        }
        
        case JsonPatchOperation::Type::MOVE: {
            json value = get_value_at_path(document, op.from);
            json temp = remove_value_at_path(document, op.from);
            return set_value_at_path(temp, op.path, value);
        }
        
        case JsonPatchOperation::Type::COPY: {
            json value = get_value_at_path(document, op.from);
            return set_value_at_path(document, op.path, value);
        }
        
        case JsonPatchOperation::Type::TEST: {
            json current_value = get_value_at_path(document, op.path);
            if (current_value != op.value) {
                throw JsonPatchException("Test operation failed at path: " + op.path);
            }
            return document; // No change for test operations
        }
    }
    
    throw JsonPatchException("Unknown operation type");
}

json JsonPatch::get_value_at_path(const json& document, const std::string& path) const {
    if (path.empty() || path == "/") {
        return document;
    }
    
    auto components = split_path(path);
    json current = document;
    
    for (const auto& component : components) {
        if (current.is_object()) {
            auto it = current.find(component);
            if (it == current.end()) {
                throw JsonPatchException("Path not found: " + path);
            }
            current = it.value();
        } else if (current.is_array()) {
            try {
                size_t index = std::stoull(component);
                if (index >= current.size()) {
                    throw JsonPatchException("Array index out of bounds: " + path);
                }
                current = current[index];
            } catch (const std::exception&) {
                throw JsonPatchException("Invalid array index: " + component);
            }
        } else {
            throw JsonPatchException("Cannot traverse path on non-container type: " + path);
        }
    }
    
    return current;
}

json JsonPatch::set_value_at_path(const json& document, const std::string& path, const json& value) const {
    if (path.empty() || path == "/") {
        return value;
    }
    
    auto components = split_path(path);
    json result = document;
    json* current = &result;
    
    for (size_t i = 0; i < components.size(); ++i) {
        const std::string& component = components[i];
        bool is_last = (i == components.size() - 1);
        
        if (current->is_object()) {
            if (is_last) {
                (*current)[component] = value;
            } else {
                if (current->find(component) == current->end()) {
                    // Create intermediate object or array based on next component
                    if (i + 1 < components.size() && 
                        std::regex_match(components[i + 1], std::regex("^\\d+$"))) {
                        (*current)[component] = json::array();
                    } else {
                        (*current)[component] = json::object();
                    }
                }
                current = &(*current)[component];
            }
        } else if (current->is_array()) {
            try {
                size_t index = std::stoull(component);
                
                // Extend array if necessary
                while (current->size() <= index) {
                    current->push_back(json{});
                }
                
                if (is_last) {
                    (*current)[index] = value;
                } else {
                    if ((*current)[index].is_null()) {
                        // Create intermediate object or array
                        if (i + 1 < components.size() && 
                            std::regex_match(components[i + 1], std::regex("^\\d+$"))) {
                            (*current)[index] = json::array();
                        } else {
                            (*current)[index] = json::object();
                        }
                    }
                    current = &(*current)[index];
                }
            } catch (const std::exception&) {
                throw JsonPatchException("Invalid array index: " + component);
            }
        } else {
            throw JsonPatchException("Cannot set value on non-container type at path: " + path);
        }
    }
    
    return result;
}

json JsonPatch::remove_value_at_path(const json& document, const std::string& path) const {
    if (path.empty() || path == "/") {
        throw JsonPatchException("Cannot remove root document");
    }
    
    auto components = split_path(path);
    if (components.empty()) {
        throw JsonPatchException("Invalid path for removal: " + path);
    }
    
    json result = document;
    json* current = &result;
    
    // Navigate to parent
    for (size_t i = 0; i < components.size() - 1; ++i) {
        const std::string& component = components[i];
        
        if (current->is_object()) {
            auto it = current->find(component);
            if (it == current->end()) {
                throw JsonPatchException("Path not found for removal: " + path);
            }
            current = &it.value();
        } else if (current->is_array()) {
            try {
                size_t index = std::stoull(component);
                if (index >= current->size()) {
                    throw JsonPatchException("Array index out of bounds for removal: " + path);
                }
                current = &(*current)[index];
            } catch (const std::exception&) {
                throw JsonPatchException("Invalid array index: " + component);
            }
        } else {
            throw JsonPatchException("Cannot traverse path for removal: " + path);
        }
    }
    
    // Remove the final component
    const std::string& final_component = components.back();
    if (current->is_object()) {
        if (current->find(final_component) == current->end()) {
            throw JsonPatchException("Key not found for removal: " + path);
        }
        current->erase(final_component);
    } else if (current->is_array()) {
        try {
            size_t index = std::stoull(final_component);
            if (index >= current->size()) {
                throw JsonPatchException("Array index out of bounds for removal: " + path);
            }
            current->erase(current->begin() + index);
        } catch (const std::exception&) {
            throw JsonPatchException("Invalid array index for removal: " + final_component);
        }
    } else {
        throw JsonPatchException("Cannot remove from non-container type: " + path);
    }
    
    return result;
}

bool JsonPatch::dry_run(const json& document) const {
    try {
        apply(document);
        return true;
    } catch (const JsonPatchException&) {
        return false;
    }
}

json JsonPatch::to_json() const {
    json result = json::array();
    for (const auto& op : operations_) {
        result.push_back(op.to_json());
    }
    return result;
}

JsonPatch JsonPatch::from_json(const json& j) {
    if (!j.is_array()) {
        throw JsonPatchException("Patch JSON must be an array");
    }
    
    std::vector<JsonPatchOperation> operations;
    for (const auto& op_json : j) {
        operations.push_back(JsonPatchOperation::from_json(op_json));
    }
    
    return JsonPatch(std::move(operations));
}

JsonPatch JsonPatch::invert(const json& original_document) const {
    std::vector<JsonPatchOperation> inverted_ops;
    
    // Process operations in reverse order
    for (auto it = operations_.rbegin(); it != operations_.rend(); ++it) {
        const auto& op = *it;
        
        switch (op.op) {
            case JsonPatchOperation::Type::ADD:
                inverted_ops.emplace_back(JsonPatchOperation::Type::REMOVE, op.path);
                break;
                
            case JsonPatchOperation::Type::REMOVE: {
                json original_value = get_value_at_path(original_document, op.path);
                inverted_ops.emplace_back(JsonPatchOperation::Type::ADD, op.path, original_value);
                break;
            }
            
            case JsonPatchOperation::Type::REPLACE: {
                json original_value = get_value_at_path(original_document, op.path);
                inverted_ops.emplace_back(JsonPatchOperation::Type::REPLACE, op.path, original_value);
                break;
            }
            
            case JsonPatchOperation::Type::MOVE:
                inverted_ops.emplace_back(JsonPatchOperation::Type::MOVE, op.path, op.from);
                break;
                
            case JsonPatchOperation::Type::COPY:
                inverted_ops.emplace_back(JsonPatchOperation::Type::REMOVE, op.path);
                break;
                
            case JsonPatchOperation::Type::TEST:
                // Test operations don't need inversion
                inverted_ops.push_back(op);
                break;
        }
    }
    
    return JsonPatch(std::move(inverted_ops));
}

bool JsonPatch::has_conflict(const json& document) const {
    return !dry_run(document);
}

JsonPatch JsonPatch::compact() const {
    // Simple compaction: remove redundant operations
    // This is a basic implementation - more sophisticated logic could be added
    std::vector<JsonPatchOperation> compacted;
    
    for (const auto& op : operations_) {
        // For now, just copy all operations
        // Future enhancement: merge consecutive operations on same path
        compacted.push_back(op);
    }
    
    return JsonPatch(std::move(compacted));
}

std::vector<std::string> JsonPatch::split_path(const std::string& path) {
    if (path.empty() || path == "/") {
        return {};
    }
    
    if (path[0] != '/') {
        throw JsonPatchException("Path must start with '/': " + path);
    }
    
    std::vector<std::string> components;
    std::stringstream ss(path.substr(1)); // Skip leading '/'
    std::string component;
    
    while (std::getline(ss, component, '/')) {
        components.push_back(unescape_path_component(component));
    }
    
    return components;
}

std::string JsonPatch::join_path(const std::vector<std::string>& components) {
    if (components.empty()) {
        return "/";
    }
    
    std::string result;
    for (const auto& component : components) {
        result += "/" + escape_path_component(component);
    }
    
    return result;
}

std::string JsonPatch::escape_path_component(const std::string& component) {
    std::string result;
    result.reserve(component.size() * 2); // Worst case estimation
    
    for (char c : component) {
        if (c == '~') {
            result += "~0";
        } else if (c == '/') {
            result += "~1";
        } else {
            result += c;
        }
    }
    
    return result;
}

std::string JsonPatch::unescape_path_component(const std::string& component) {
    std::string result;
    result.reserve(component.size());
    
    for (size_t i = 0; i < component.size(); ++i) {
        if (component[i] == '~' && i + 1 < component.size()) {
            if (component[i + 1] == '0') {
                result += '~';
                ++i;
            } else if (component[i + 1] == '1') {
                result += '/';
                ++i;
            } else {
                result += component[i];
            }
        } else {
            result += component[i];
        }
    }
    
    return result;
}

// Convenience function
JsonPatch diff(const json& from, const json& to, const JsonDiffOptions& options) {
    return JsonPatch::diff(from, to, options);
}

