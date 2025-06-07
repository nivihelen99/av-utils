#pragma once

#include <functional>
#include <stack>
#include <memory>
#include <vector>
#include <string>
#include <iostream>

namespace undo_system {

// Simple command interface
class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
};

// Generic command implementation
class GenericCommand : public Command {
private:
    std::function<void()> execute_func_;
    std::function<void()> undo_func_;

public:
    GenericCommand(std::function<void()> exec, std::function<void()> undo)
        : execute_func_(std::move(exec)), undo_func_(std::move(undo)) {}

    void execute() override { execute_func_(); }
    void undo() override { undo_func_(); }
};

// Main UndoManager class
class UndoManager {
private:
    std::stack<std::unique_ptr<Command>> undo_stack_;
    std::stack<std::unique_ptr<Command>> redo_stack_;

public:
    // Execute a command and add it to undo stack
    void execute_command(std::unique_ptr<Command> cmd) {
        cmd->execute();
        undo_stack_.push(std::move(cmd));
        
        // Clear redo stack when new command is executed
        while (!redo_stack_.empty()) {
            redo_stack_.pop();
        }
    }

    // Create and execute a simple command
    void execute(std::function<void()> exec_func, std::function<void()> undo_func) {
        auto cmd = std::make_unique<GenericCommand>(std::move(exec_func), std::move(undo_func));
        execute_command(std::move(cmd));
    }

    // Undo the last command
    bool undo() {
        if (undo_stack_.empty()) {
            return false;
        }
        
        auto cmd = std::move(undo_stack_.top());
        undo_stack_.pop();
        
        cmd->undo();
        redo_stack_.push(std::move(cmd));
        return true;
    }

    // Redo the last undone command
    bool redo() {
        if (redo_stack_.empty()) {
            return false;
        }
        
        auto cmd = std::move(redo_stack_.top());
        redo_stack_.pop();
        
        cmd->execute();
        undo_stack_.push(std::move(cmd));
        return true;
    }

    // Check availability
    bool can_undo() const { return !undo_stack_.empty(); }
    bool can_redo() const { return !redo_stack_.empty(); }
    
    // Get counts
    size_t undo_count() const { return undo_stack_.size(); }
    size_t redo_count() const { return redo_stack_.size(); }
    
    // Clear all history
    void clear() {
        while (!undo_stack_.empty()) undo_stack_.pop();
        while (!redo_stack_.empty()) redo_stack_.pop();
    }
};

// Undoable value wrapper
template<typename T>
class UndoableValue {
private:
    T value_;
    UndoManager* manager_;

public:
    explicit UndoableValue(const T& initial_value, UndoManager* mgr = nullptr)
        : value_(initial_value), manager_(mgr) {}

    void set(const T& new_value) {
        if (manager_ && new_value != value_) {
            T old_value = value_;
            manager_->execute(
                [this, new_value]() { value_ = new_value; },
                [this, old_value]() { value_ = old_value; }
            );
        } else {
            value_ = new_value;
        }
    }

    const T& get() const { return value_; }
    operator const T&() const { return value_; }
    
    UndoableValue& operator=(const T& new_value) {
        set(new_value);
        return *this;
    }

    void set_manager(UndoManager* mgr) { manager_ = mgr; }
};

// Undoable container wrapper
template<typename Container>
class UndoableContainer {
private:
    Container container_;
    UndoManager* manager_;

public:
    explicit UndoableContainer(UndoManager* mgr = nullptr)
        : manager_(mgr) {}

    explicit UndoableContainer(const Container& initial, UndoManager* mgr = nullptr)
        : container_(initial), manager_(mgr) {}

    void push_back(const typename Container::value_type& value) {
        if (manager_) {
            manager_->execute(
                [this, value]() { container_.push_back(value); },
                [this]() { 
                    if (!container_.empty()) {
                        container_.pop_back(); 
                    }
                }
            );
        } else {
            container_.push_back(value);
        }
    }

    void push_back(typename Container::value_type&& value) {
        if (manager_) {
            // Make a copy for the lambda
            auto value_copy = value;
            manager_->execute(
                [this, val = std::move(value)]() mutable { 
                    container_.push_back(std::move(val)); 
                },
                [this]() { 
                    if (!container_.empty()) {
                        container_.pop_back(); 
                    }
                }
            );
        } else {
            container_.push_back(std::move(value));
        }
    }

    void pop_back() {
        if (container_.empty()) return;
        
        if (manager_) {
            auto last_element = container_.back();
            manager_->execute(
                [this]() { container_.pop_back(); },
                [this, last_element]() { container_.push_back(last_element); }
            );
        } else {
            container_.pop_back();
        }
    }

    // Access methods
    Container& get() { return container_; }
    const Container& get() const { return container_; }
    
    auto size() const { return container_.size(); }
    bool empty() const { return container_.empty(); }
    auto begin() { return container_.begin(); }
    auto end() { return container_.end(); }
    auto begin() const { return container_.begin(); }
    auto end() const { return container_.end(); }
    
    auto& operator[](size_t index) { return container_[index]; }
    const auto& operator[](size_t index) const { return container_[index]; }

    void set_manager(UndoManager* mgr) { manager_ = mgr; }
};

} // namespace undo_system

// Example usage and demonstrations
namespace examples {

struct Point {
    int x, y;
    
    Point(int x = 0, int y = 0) : x(x), y(y) {}
    
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
    
    bool operator!=(const Point& other) const {
        return !(*this == other);
    }
};

std::ostream& operator<<(std::ostream& os, const Point& p) {
    return os << "Point(" << p.x << ", " << p.y << ")";
}

void demonstrate_basic_usage() {
    std::cout << "=== Basic Undo/Redo Demonstration ===\n";
    
    undo_system::UndoManager manager;
    int value = 10;
    
    std::cout << "Initial value: " << value << "\n";
    
    // Double the value
    manager.execute(
        [&value]() { value *= 2; },
        [&value]() { value /= 2; }
    );
    std::cout << "After doubling: " << value << "\n";
    
    // Add 5
    manager.execute(
        [&value]() { value += 5; },
        [&value]() { value -= 5; }
    );
    std::cout << "After adding 5: " << value << "\n";
    
    // Undo operations
    std::cout << "Undoing...\n";
    manager.undo();
    std::cout << "After first undo: " << value << "\n";
    manager.undo();
    std::cout << "After second undo: " << value << "\n";
    
    // Redo operations
    std::cout << "Redoing...\n";
    manager.redo();
    std::cout << "After first redo: " << value << "\n";
    manager.redo();
    std::cout << "After second redo: " << value << "\n";
    
    std::cout << "\n";
}

void demonstrate_undoable_value() {
    std::cout << "=== UndoableValue Demonstration ===\n";
    
    undo_system::UndoManager manager;
    
    undo_system::UndoableValue<int> int_val(42, &manager);
    std::cout << "Initial int value: " << int_val.get() << "\n";
    
    int_val.set(100);
    std::cout << "After setting to 100: " << int_val.get() << "\n";
    
    int_val = 200;
    std::cout << "After assignment to 200: " << int_val.get() << "\n";
    
    undo_system::UndoableValue<std::string> str_val("Hello", &manager);
    str_val.set("World");
    std::cout << "String value: " << str_val.get() << "\n";
    
    undo_system::UndoableValue<Point> point_val(Point(1, 2), &manager);
    point_val.set(Point(5, 10));
    std::cout << "Point value: " << point_val.get() << "\n";
    
    std::cout << "Undoing changes...\n";
    while (manager.can_undo()) {
        manager.undo();
        std::cout << "Int: " << int_val.get() 
                  << ", String: " << str_val.get() 
                  << ", Point: " << point_val.get() << "\n";
    }
    
    std::cout << "\n";
}

void demonstrate_undoable_container() {
    std::cout << "=== UndoableContainer Demonstration ===\n";
    
    undo_system::UndoManager manager;
    undo_system::UndoableContainer<std::vector<int>> vec(&manager);
    
    std::cout << "Initial size: " << vec.size() << "\n";
    
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    
    std::cout << "After adding elements: ";
    for (const auto& elem : vec) {
        std::cout << elem << " ";
    }
    std::cout << "(size: " << vec.size() << ")\n";
    
    vec.pop_back();
    std::cout << "After pop_back: ";
    for (const auto& elem : vec) {
        std::cout << elem << " ";
    }
    std::cout << "(size: " << vec.size() << ")\n";
    
    std::cout << "Undoing operations...\n";
    while (manager.can_undo()) {
        manager.undo();
        std::cout << "Size: " << vec.size() << ", Elements: ";
        for (const auto& elem : vec) {
            std::cout << elem << " ";
        }
        std::cout << "\n";
    }
    
    std::cout << "\n";
}

void demonstrate_complex_scenario() {
    std::cout << "=== Complex Scenario Demonstration ===\n";
    
    undo_system::UndoManager manager;
    
    undo_system::UndoableValue<std::string> name("John", &manager);
    undo_system::UndoableValue<int> age(25, &manager);
    undo_system::UndoableContainer<std::vector<std::string>> hobbies(&manager);
    
    auto print_state = [&]() {
        std::cout << "Name: " << name.get() 
                  << ", Age: " << age.get() 
                  << ", Hobbies: ";
        for (const auto& hobby : hobbies) {
            std::cout << hobby << " ";
        }
        std::cout << "\n";
    };
    
    std::cout << "Initial state:\n";
    print_state();
    
    name.set("Jane");
    age.set(30);
    hobbies.push_back("Reading");
    hobbies.push_back("Gaming");
    
    std::cout << "\nAfter changes:\n";
    print_state();
    
    // Compound operation
    manager.execute(
        [&]() {
            name.set("Anonymous");
            age.set(0);
        },
        [&]() {
            name.set("Jane");
            age.set(30);
        }
    );
    
    std::cout << "\nAfter compound operation:\n";
    print_state();
    
    std::cout << "\nUndoing step by step:\n";
    while (manager.can_undo()) {
        manager.undo();
        print_state();
    }
    
    std::cout << "\nRedo everything:\n";
    while (manager.can_redo()) {
        manager.redo();
        print_state();
    }
    
    std::cout << "\n";
}

} // namespace examples

/*
int main() {
    examples::demonstrate_basic_usage();
    examples::demonstrate_undoable_value();
    examples::demonstrate_undoable_container();
    examples::demonstrate_complex_scenario();
    
    return 0;
}
*/
