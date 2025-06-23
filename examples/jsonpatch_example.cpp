#include "jsonpatch.h" // Should be relative to include path
#include <iostream>
#include <cassert>

// Using a namespace or prefix for json might be good if json.hpp doesn't do it.
// Assuming 'json' is nlohmann::json as in jsonpatch.h
using json = nlohmann::json;

void basic_example() {
    std::cout << "=== Basic JsonPatch Example ===\n";

    json a = {
        {"version", 1},
        {"config", {
            {"ip", "192.168.1.1"},
            {"enabled", true}
        }}
    };

    json b = {
        {"version", 2},
        {"config", {
            {"ip", "10.0.0.1"},
            {"enabled", false},
            {"mode", "advanced"}
        }}
    };

    // Generate patch
    JsonPatch patch = diff(a, b);

    std::cout << "Original document:\n" << a.dump(2) << "\n\n";
    std::cout << "Target document:\n" << b.dump(2) << "\n\n";
    std::cout << "Generated patch:\n" << patch.to_json().dump(2) << "\n\n";

    // Apply patch
    json patched = patch.apply(a);

    std::cout << "Patched document:\n" << patched.dump(2) << "\n\n";

    // Verify result
    assert(patched == b);
    std::cout << "? Patch applied successfully!\n\n";
}

void array_example() {
    std::cout << "=== Array Patch Example ===\n";

    json original = {
        {"items", {"apple", "banana", "cherry"}},
        {"count", 3}
    };

    json modified = {
        {"items", {"apple", "blueberry", "cherry", "date"}},
        {"count", 4}
    };

    JsonPatch patch = diff(original, modified);

    std::cout << "Array patch operations:\n" << patch.to_json().dump(2) << "\n\n";

    json result = patch.apply(original);
    assert(result == modified);
    std::cout << "? Array patch applied successfully!\n\n";
}

void serialization_example() {
    std::cout << "=== Serialization Example ===\n";

    json doc1 = {{"name", "John"}, {"age", 30}};
    json doc2 = {{"name", "Jane"}, {"age", 25}, {"city", "NYC"}};

    // Create patch
    JsonPatch original_patch = diff(doc1, doc2);

    // Serialize to string
    std::string patch_json_str = original_patch.to_json().dump(); // Renamed to avoid conflict
    std::cout << "Serialized patch: " << patch_json_str << "\n\n";

    // Deserialize from string
    JsonPatch loaded_patch = JsonPatch::from_json(json::parse(patch_json_str));

    // Apply loaded patch
    json result = loaded_patch.apply(doc1);
    assert(result == doc2);
    std::cout << "? Serialization/deserialization works!\n\n";
}

void inversion_example() {
    std::cout << "=== Patch Inversion Example ===\n";

    json original = {{"x", 10}, {"y", 20}};
    json modified = {{"x", 15}, {"z", 30}};

    // Create forward patch
    JsonPatch forward_patch = diff(original, modified);
    std::cout << "Forward patch:\n" << forward_patch.to_json().dump(2) << "\n\n";

    // Apply forward patch
    json result = forward_patch.apply(original);

    // Create inverse patch
    JsonPatch inverse_patch = forward_patch.invert(original);
    std::cout << "Inverse patch:\n" << inverse_patch.to_json().dump(2) << "\n\n";

    // Apply inverse patch to get back original
    json restored = inverse_patch.apply(result);

    std::cout << "Original:  " << original.dump() << "\n";
    std::cout << "Modified:  " << result.dump() << "\n";
    std::cout << "Restored:  " << restored.dump() << "\n\n";

    assert(restored == original);
    std::cout << "? Patch inversion works (undo functionality)!\n\n";
}


int main()
{
    basic_example();
    array_example();
    serialization_example();
    inversion_example();

    return 0;
}
