#include "../include/tagged_union.h" // Adjust path as necessary
#include <iostream>
#include <string>
#include <vector>

// Example custom struct
struct Point {
    int x, y;
};

// Provide a type name for Point
template<>
struct type_name_trait<Point> {
    static constexpr std::string_view tag = "Point";
};

// Example event types
struct MouseClickEvent {
    int x, y;
    int button;
};
template<> struct type_name_trait<MouseClickEvent> { static constexpr std::string_view tag = "MouseClickEvent"; };

struct KeyPressEvent {
    int key_code;
    char character;
};
template<> struct type_name_trait<KeyPressEvent> { static constexpr std::string_view tag = "KeyPressEvent"; };

struct SystemMessageEvent {
    std::string message;
    int priority;
};
template<> struct type_name_trait<SystemMessageEvent> { static constexpr std::string_view tag = "SystemMessageEvent"; };


// Function to process a generic event stored in TaggedUnion
void process_event(const TaggedUnion& event_data) {
    std::cout << "\nProcessing event with tag: '" << event_data.type_tag() << "'" << std::endl;

    if (const auto* click_event = event_data.get_if<MouseClickEvent>()) {
        std::cout << "  MouseClickEvent: x=" << click_event->x
                  << ", y=" << click_event->y
                  << ", button=" << click_event->button << std::endl;
    } else if (const auto* key_event = event_data.get_if<KeyPressEvent>()) {
        std::cout << "  KeyPressEvent: key_code=" << key_event->key_code
                  << ", char='" << key_event->character << "'" << std::endl;
    } else if (const auto* sys_event = event_data.get_if<SystemMessageEvent>()) {
        std::cout << "  SystemMessageEvent: message=\"" << sys_event->message
                  << "\", priority=" << sys_event->priority << std::endl;
    } else if (event_data.has_value()) {
        std::cout << "  Unknown event type encountered." << std::endl;
    } else {
        std::cout << "  Event data is empty." << std::endl;
    }
}

int main() {
    std::cout << "--- TaggedUnion Basic Example ---" << std::endl;

    TaggedUnion data;

    // Store an integer
    data.set(42);
    std::cout << "Stored tag: " << data.type_tag() << std::endl; // Expected: "int" or "i"
    if (auto* val = data.get_if<int>()) {
        std::cout << "Retrieved int: " << *val << std::endl;
    }

    // Store a string
    data.set(std::string("Hello, TaggedUnion!"));
    std::cout << "Stored tag: " << data.type_tag() << std::endl; // Expected: "std::string"
    if (auto* str = data.get_if<std::string>()) {
        std::cout << "Retrieved string: \"" << *str << "\"" << std::endl;
    }
     // Store a C-style string literal
    const char* c_str_literal = "C-string literal";
    data.set(c_str_literal); // This will store a const char*
    std::cout << "Stored tag: " << data.type_tag() << std::endl; // Expected: typeid(const char*).name() or Pkc
    if (auto* c_str_val = data.get_if<const char*>()) {
        std::cout << "Retrieved const char*: \"" << *c_str_val << "\"" << std::endl;
    }


    // Store a custom struct Point
    data.set(Point{10, 20});
    std::cout << "Stored tag: " << data.type_tag() << std::endl; // Expected: "Point"
    if (auto* pt = data.get_if<Point>()) {
        std::cout << "Retrieved Point: x=" << pt->x << ", y=" << pt->y << std::endl;
    }

    // Demonstrate trying to get the wrong type
    std::cout << "Trying to get int (should be nullptr): "
              << (data.get_if<int>() == nullptr ? "nullptr (correct)" : "non-nullptr (WRONG)")
              << std::endl;

    // Reset the TaggedUnion
    data.reset();
    std::cout << "After reset, has_value: " << data.has_value() << std::endl; // Expected: 0 (false)
    std::cout << "After reset, tag: " << data.type_tag() << std::endl; // Expected: "empty"

    std::cout << "\n--- Event System Example ---" << std::endl;
    std::vector<TaggedUnion> event_queue;

    // Create and add events
    TaggedUnion event1;
    event1.set(MouseClickEvent{100, 200, 1});
    event_queue.push_back(std::move(event1)); // Test move construction into vector

    TaggedUnion event2;
    event2.set(KeyPressEvent{32, ' '}); // Spacebar
    event_queue.push_back(std::move(event2));

    TaggedUnion event3;
    event3.set(SystemMessageEvent{"System rebooting soon.", 1});
    event_queue.push_back(std::move(event3));

    TaggedUnion event4; // An empty event
    event_queue.push_back(std::move(event4));

    TaggedUnion event5;
    event5.set(Point{1,2}); // Not an "event" type known to process_event
    event_queue.push_back(std::move(event5));


    // Process events
    for (const auto& current_event : event_queue) {
        process_event(current_event);
    }

    // Example of direct set with rvalue
    TaggedUnion rvalue_tu;
    rvalue_tu.set(MouseClickEvent{5,5,0});
    process_event(rvalue_tu);


    std::cout << "\n--- TaggedUnion Example Finished ---" << std::endl;

    return 0;
}
