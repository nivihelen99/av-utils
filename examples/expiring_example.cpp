#include "expiring_containers.h"
#include <iostream>
#include <thread>
#include <cassert>
#include <string>

using namespace std::chrono_literals;
using namespace expiring;

void test_timestamped_queue() {
    std::cout << "=== Testing TimeStampedQueue ===\n";
    
    // Create a queue with 2-second TTL
    TimeStampedQueue<std::string> queue(2000ms);
    
    // Test basic operations
    queue.push("first");
    queue.push("second");
    queue.push("third");
    
    std::cout << "Queue size after 3 pushes: " << queue.size() << std::endl;
    assert(queue.size() == 3);
    
    // Test front access
    std::cout << "Front element: " << queue.front() << std::endl;
    assert(queue.front() == "first");
    
    // Test pop
    std::string popped = queue.pop();
    std::cout << "Popped element: " << popped << std::endl;
    assert(popped == "first");
    assert(queue.size() == 2);
    
    // Wait for expiration
    std::cout << "Waiting 2.5 seconds for expiration...\n";
    std::this_thread::sleep_for(2500ms);
    
    std::cout << "Queue size after expiration: " << queue.size() << std::endl;
    assert(queue.size() == 0);
    assert(queue.empty());
    
    // Test with new entries after expiration
    queue.push("new_entry");
    assert(queue.size() == 1);
    assert(queue.front() == "new_entry");
    
    std::cout << "TimeStampedQueue tests passed!\n\n";
}

void test_expiring_dict() {
    std::cout << "=== Testing ExpiringDict ===\n";
    
    // Create a dict with 1.5-second TTL
    ExpiringDict<std::string, int> cache(1500ms);
    
    // Test basic operations
    cache.insert("foo", 42);
    cache.insert("bar", 99);
    cache.insert("baz", 123);
    
    std::cout << "Cache size after 3 inserts: " << cache.size() << std::endl;
    assert(cache.size() == 3);
    
    // Test find operations
    auto* foo_val = cache.find("foo");
    assert(foo_val != nullptr);
    std::cout << "Found foo: " << *foo_val << std::endl;
    assert(*foo_val == 42);
    
    // Test contains
    assert(cache.contains("bar"));
    assert(!cache.contains("nonexistent"));
    
    // Test update
    bool existed = cache.update("foo", 84);
    assert(existed);
    foo_val = cache.find("foo");
    assert(*foo_val == 84);
    
    // Wait for partial expiration
    std::cout << "Waiting 1 second...\n";
    std::this_thread::sleep_for(1000ms);
    
    // Add new entry while others are about to expire
    cache.insert("new", 456);
    
    std::cout << "Waiting another 1 second for expiration...\n";
    std::this_thread::sleep_for(1000ms);
    
    // Original entries should be expired, new one should remain
    std::cout << "Cache size after expiration: " << cache.size() << std::endl;
    assert(cache.size() == 1);
    assert(cache.contains("new"));
    assert(!cache.contains("foo"));
    assert(!cache.contains("bar"));
    
    std::cout << "ExpiringDict tests passed!\n\n";
}

void test_access_renews_ttl() {
    std::cout << "=== Testing Access Renews TTL ===\n";
    
    // Create dict with access-based TTL renewal
    ExpiringDict<std::string, int> cache(1000ms, true);
    
    cache.insert("refresh_me", 777);
    
    // Keep accessing the entry to renew its TTL
    for (int i = 0; i < 3; ++i) {
        std::this_thread::sleep_for(800ms);
        std::cout << "Accessing entry (iteration " << (i+1) << ")...\n";
        assert(cache.contains("refresh_me"));
        auto* val = cache.find("refresh_me");
        assert(val != nullptr && *val == 777);
    }
    
    // Now stop accessing and let it expire
    std::cout << "Waiting 1.2 seconds without access...\n";
    std::this_thread::sleep_for(1200ms);
    
    assert(!cache.contains("refresh_me"));
    std::cout << "Entry expired after no access!\n";
    
    std::cout << "Access renews TTL test passed!\n\n";
}

void test_rate_limiting_example() {
    std::cout << "=== Rate Limiting Example ===\n";
    
    // Rate limiter: allow max 3 requests per 2 seconds per client
    TimeStampedQueue<std::string> rate_limiter(2000ms);
    const size_t max_requests = 3;
    
    auto check_rate_limit = [&](const std::string& client_id) -> bool {
        // Count current requests for this client
        size_t request_count = 0;
        TimeStampedQueue<std::string> temp_queue(2000ms);
        
        // We'd normally need a per-client queue, but for demo we'll simulate
        while (!rate_limiter.empty()) {
            std::string req = rate_limiter.pop();
            if (req == client_id) {
                request_count++;
            }
            temp_queue.push(req);
        }
        
        // Restore queue (in real implementation, we'd use per-client queues)
        while (!temp_queue.empty()) {
            rate_limiter.push(temp_queue.pop());
        }
        
        if (request_count >= max_requests) {
            return false; // Rate limited
        }
        
        rate_limiter.push(client_id);
        return true; // Allowed
    };
    
    // Simulate requests
    std::cout << "Client 'user1' making requests:\n";
    for (int i = 1; i <= 5; ++i) {
        bool allowed = check_rate_limit("user1");
        std::cout << "Request " << i << ": " << (allowed ? "ALLOWED" : "RATE LIMITED") << std::endl;
        std::this_thread::sleep_for(200ms);
    }
    
    std::cout << "Rate limiting example completed!\n\n";
}

void test_event_deduplication() {
    std::cout << "=== Event Deduplication Example ===\n";
    
    ExpiringDict<std::string, bool> recent_events(3000ms);
    
    auto emit_alarm = [](const std::string& event) {
        std::cout << "ALARM: " << event << std::endl;
    };
    
    auto process_event = [&](const std::string& event) {
        if (!recent_events.contains(event)) {
            recent_events.insert(event, true);
            emit_alarm(event);
        } else {
            std::cout << "Duplicate event ignored: " << event << std::endl;
        }
    };
    
    // Simulate events
    process_event("link-flap");
    process_event("high-cpu");
    process_event("link-flap");  // Duplicate, should be ignored
    process_event("disk-full");
    
    std::this_thread::sleep_for(1000ms);
    process_event("link-flap");  // Still duplicate
    
    std::this_thread::sleep_for(2500ms);  // Let events expire
    process_event("link-flap");  // Should trigger alarm again
    
    std::cout << "Event deduplication example completed!\n\n";
}

void test_for_each_visitor() {
    std::cout << "=== Testing for_each Visitor ===\n";
    
    ExpiringDict<std::string, int> cache(1000ms);
    
    cache.insert("a", 1);
    cache.insert("b", 2);
    cache.insert("c", 3);
    
    std::cout << "Current cache contents:\n";
    cache.for_each([](const std::string& key, const int& value) {
        std::cout << "  " << key << " => " << value << std::endl;
    });
    
    std::this_thread::sleep_for(1200ms);  // Let entries expire
    
    std::cout << "Cache contents after expiration:\n";
    cache.for_each([](const std::string& key, const int& value) {
        std::cout << "  " << key << " => " << value << std::endl;
    });
    std::cout << "(No output expected - all expired)\n";
    
    std::cout << "for_each visitor test passed!\n\n";
}

int main() {
    try {
        test_timestamped_queue();
        test_expiring_dict();
        test_access_renews_ttl();
        test_rate_limiting_example();
        test_event_deduplication();
        test_for_each_visitor();
        
        std::cout << "All tests passed successfully!\n";
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
