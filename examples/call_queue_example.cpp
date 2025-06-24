#include "call_queue.h"
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <sstream>

using namespace callqueue;

// Helper function for printing with thread ID
std::string thread_prefix() {
    std::ostringstream oss;
    oss << "[Thread " << std::this_thread::get_id() << "] ";
    return oss.str();
}

void demo_basic_usage() {
    std::cout << "\n=== Basic Usage Demo ===\n";
    
    CallQueue queue;
    
    // Simple lambda queuing
    queue.push([] { std::cout << "Hello "; });
    queue.push([] { std::cout << "World!\n"; });
    
    std::cout << "Queue size before drain: " << queue.size() << "\n";
    queue.drain_all();
    std::cout << "Queue size after drain: " << queue.size() << "\n";
}

void demo_captured_state() {
    std::cout << "\n=== Captured State Demo ===\n";
    
    CallQueue queue;
    std::vector<int> results;
    int counter = 0;
    
    // Capture by reference
    queue.push([&results, &counter]() { 
        results.push_back(++counter); 
        std::cout << "Added " << counter << " to results\n";
    });
    
    queue.push([&results, &counter]() { 
        results.push_back(++counter); 
        std::cout << "Added " << counter << " to results\n";
    });
    
    queue.push([&results]() {
        std::cout << "Results vector contains: ";
        for (int val : results) {
            std::cout << val << " ";
        }
        std::cout << "\n";
    });
    
    queue.drain_all();
}

void demo_gui_update_batching() {
    std::cout << "\n=== GUI Update Batching Demo ===\n";
    
    struct MockWidget {
        std::string name;
        bool needs_redraw = false;
        
        MockWidget(std::string n) : name(std::move(n)) {}
        
        void invalidate() {
            needs_redraw = true;
            std::cout << "Widget '" << name << "' marked for redraw\n";
        }
        
        void redraw() {
            if (needs_redraw) {
                std::cout << "Redrawing widget '" << name << "'\n";
                needs_redraw = false;
            }
        }
    };
    
    CallQueue ui_queue;
    MockWidget button("Button1");
    MockWidget label("Label1");
    MockWidget panel("Panel1");
    
    // Simulate various UI events that trigger redraws
    std::cout << "Scheduling UI updates...\n";
    ui_queue.push([&button]() { button.invalidate(); });
    ui_queue.push([&label]() { label.invalidate(); });
    ui_queue.push([&panel]() { panel.invalidate(); });
    
    // Batch all redraws at once
    ui_queue.push([&button, &label, &panel]() {
        std::cout << "Executing batched redraws:\n";
        button.redraw();
        label.redraw();
        panel.redraw();
    });
    
    std::cout << "Processing UI queue...\n";
    ui_queue.drain_all();
}

void demo_state_machine() {
    std::cout << "\n=== State Machine Demo ===\n";
    
    enum class State { IDLE, PROCESSING, COMPLETE, ERROR };
    
    struct StateMachine {
        State current_state = State::IDLE;
        CallQueue side_effects;
        
        void transition_to(State new_state) {
            std::cout << "Transitioning from " << state_name(current_state) 
                      << " to " << state_name(new_state) << "\n";
            current_state = new_state;
        }
        
        std::string state_name(State s) {
            switch (s) {
                case State::IDLE: return "IDLE";
                case State::PROCESSING: return "PROCESSING";
                case State::COMPLETE: return "COMPLETE";
                case State::ERROR: return "ERROR";
            }
            return "UNKNOWN";
        }
    };
    
    StateMachine fsm;
    
    // Queue side effects for state transitions
    fsm.side_effects.push([&fsm]() {
        fsm.transition_to(State::PROCESSING);
    });
    
    fsm.side_effects.push([&fsm]() {
        std::cout << "Performing processing work...\n";
        // Simulate work
    });
    
    fsm.side_effects.push([&fsm]() {
        fsm.transition_to(State::COMPLETE);
    });
    
    fsm.side_effects.push([&fsm]() {
        std::cout << "Cleanup after completion\n";
    });
    
    std::cout << "Initial state: " << fsm.state_name(fsm.current_state) << "\n";
    std::cout << "Executing state machine transitions...\n";
    fsm.side_effects.drain_all();
    std::cout << "Final state: " << fsm.state_name(fsm.current_state) << "\n";
}

void demo_coalescing() {
    std::cout << "\n=== Task Coalescing Demo ===\n";
    
    CallQueue queue;
    std::string log;
    
    // Add multiple updates for the same resource - only the last one should execute
    queue.coalesce("update_config", [&log]() { 
        log += "Config update v1; "; 
    });
    
    queue.coalesce("update_config", [&log]() { 
        log += "Config update v2; "; 
    });
    
    queue.coalesce("update_config", [&log]() { 
        log += "Config update v3; "; 
    });
    
    // Different key - this should execute
    queue.coalesce("update_database", [&log]() { 
        log += "Database update; "; 
    });
    
    std::cout << "Queue size: " << queue.size() << "\n";
    queue.drain_all();
    std::cout << "Execution log: " << log << "\n";
}

void demo_drain_one() {
    std::cout << "\n=== Drain One Demo ===\n";
    
    CallQueue queue;
    
    queue.push([]() { std::cout << "Task 1\n"; });
    queue.push([]() { std::cout << "Task 2\n"; });
    queue.push([]() { std::cout << "Task 3\n"; });
    
    std::cout << "Initial queue size: " << queue.size() << "\n";
    
    std::cout << "Draining one task at a time:\n";
    while (!queue.empty()) {
        std::cout << "About to drain (size=" << queue.size() << "): ";
        queue.drain_one();
    }
    
    std::cout << "Final queue size: " << queue.size() << "\n";
}

void demo_max_size_limit() {
    std::cout << "\n=== Max Size Limit Demo ===\n";
    
    CallQueue queue(3); // Limit to 3 tasks
    
    std::cout << "Queue max size: " << queue.max_size() << "\n";
    
    bool success;
    success = queue.push([]() { std::cout << "Task 1\n"; });
    std::cout << "Added task 1: " << (success ? "SUCCESS" : "FAILED") << "\n";
    
    success = queue.push([]() { std::cout << "Task 2\n"; });
    std::cout << "Added task 2: " << (success ? "SUCCESS" : "FAILED") << "\n";
    
    success = queue.push([]() { std::cout << "Task 3\n"; });
    std::cout << "Added task 3: " << (success ? "SUCCESS" : "FAILED") << "\n";
    
    success = queue.push([]() { std::cout << "Task 4\n"; });
    std::cout << "Added task 4: " << (success ? "SUCCESS" : "FAILED") << " (should fail)\n";
    
    std::cout << "Current queue size: " << queue.size() << "\n";
    std::cout << "Draining all tasks:\n";
    queue.drain_all();
}

void demo_thread_safety() {
    std::cout << "\n=== Thread Safety Demo ===\n";
    
    ThreadSafeCallQueue queue;
    std::atomic<int> counter{0};
    const int num_threads = 4;
    const int tasks_per_thread = 10;
    
    std::vector<std::thread> producers;
    
    // Create producer threads
    for (int t = 0; t < num_threads; ++t) {
        producers.emplace_back([&queue, &counter, t, tasks_per_thread]() {
            for (int i = 0; i < tasks_per_thread; ++i) {
                queue.push([&counter, t, i]() {
                    int value = counter.fetch_add(1);
                    std::cout << thread_prefix() << "Thread " << t 
                              << ", Task " << i << ", Counter=" << value << "\n";
                });
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }
    
    // Consumer thread that periodically drains the queue
    std::thread consumer([&queue]() {
        for (int i = 0; i < 5; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            std::cout << thread_prefix() << "Draining queue (size=" 
                      << queue.size() << ")...\n";
            queue.drain_all();
        }
    });
    
    // Wait for all producers
    for (auto& t : producers) {
        t.join();
    }
    consumer.join();
    
    // Final drain
    std::cout << "Final drain (size=" << queue.size() << ")...\n";
    queue.drain_all();
    
    std::cout << "Final counter value: " << counter.load() << "\n";
}

void demo_networking_control_plane() {
    std::cout << "\n=== Network Control Plane Demo ===\n";
    
    struct NetworkUpdate {
        std::string type;
        std::string details;
        
        NetworkUpdate(std::string t, std::string d) : type(std::move(t)), details(std::move(d)) {}
    };
    
    CallQueue control_queue;
    std::vector<NetworkUpdate> applied_updates;
    
    // Simulate various network control plane updates
    control_queue.push([&applied_updates]() {
        applied_updates.emplace_back("MAC", "Add MAC entry 00:11:22:33:44:55 -> Port 1");
        std::cout << "Applied MAC table update\n";
    });
    
    control_queue.push([&applied_updates]() {
        applied_updates.emplace_back("FDB", "Update FDB for VLAN 100");
        std::cout << "Applied FDB update\n";
    });
    
    control_queue.push([&applied_updates]() {
        applied_updates.emplace_back("ACL", "Add ACL rule: permit tcp any any eq 80");
        std::cout << "Applied ACL update\n";
    });
    
    control_queue.push([&applied_updates]() {
        applied_updates.emplace_back("ROUTE", "Add route 192.168.1.0/24 -> GW 10.0.0.1");
        std::cout << "Applied routing update\n";
    });
    
    // Batch commit all updates
    control_queue.push([&applied_updates]() {
        std::cout << "\n=== Committing batch of " << applied_updates.size() << " updates ===\n";
        for (const auto& update : applied_updates) {
            std::cout << "COMMIT: [" << update.type << "] " << update.details << "\n";
        }
        std::cout << "Batch commit complete\n";
    });
    
    std::cout << "Processing network control plane updates...\n";
    control_queue.drain_all();
}

void demo_reentrancy_handling() {
    std::cout << "\n=== Reentrancy Handling Demo ===\n";
    
    CallQueue queue;
    
    queue.push([&queue]() {
        std::cout << "Task 1 executing\n";
        // This task adds more tasks during drain_all()
        queue.push([]() { std::cout << "Task added during drain (should execute next cycle)\n"; });
        queue.push([]() { std::cout << "Another task added during drain\n"; });
    });
    
    queue.push([]() {
        std::cout << "Task 2 executing\n";
    });
    
    std::cout << "Initial queue size: " << queue.size() << "\n";
    std::cout << "First drain cycle:\n";
    queue.drain_all();
    
    std::cout << "Queue size after first drain: " << queue.size() << "\n";
    std::cout << "Second drain cycle:\n";
    queue.drain_all();
    
    std::cout << "Queue size after second drain: " << queue.size() << "\n";
}

int main() {
    std::cout << "CallQueue / FunctionBuffer Use Cases Demo\n";
    std::cout << "=========================================\n";
    
    demo_basic_usage();
    demo_captured_state();
    demo_gui_update_batching();
    demo_state_machine();
    demo_coalescing();
    demo_drain_one();
    demo_max_size_limit();
    demo_thread_safety();
    demo_networking_control_plane();
    demo_reentrancy_handling();
    
    std::cout << "\n=== All demos completed ===\n";
    return 0;
}
