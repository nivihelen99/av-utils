#include "interval_counter.h"
#include <iostream>
#include <thread>
#include <random>
#include <vector>
#include <iomanip>

using namespace std::chrono_literals;
using namespace util;

// Example 1: Basic Usage
void example_basic_usage() {
    std::cout << "\n=== Example 1: Basic Usage ===\n";
    
    IntervalCounter rate_tracker(60s);  // Track events in past 60 seconds
    
    // Record some events
    rate_tracker.record();        // Record 1 event
    rate_tracker.record(5);       // Record 5 events
    rate_tracker.record(2);       // Record 2 more events
    
    std::cout << "Total events in last 60s: " << rate_tracker.count() << std::endl;
    std::cout << "Rate: " << std::fixed << std::setprecision(2) 
              << rate_tracker.rate_per_second() << " events/sec\n";
}

// Example 2: API Request Rate Monitoring
class APIServer {
private:
    IntervalCounter request_counter_;
    const size_t max_requests_per_minute_ = 100;
    
public:
    APIServer() : request_counter_(60s, 1s) {}
    
    bool handle_request(const std::string& endpoint) {
        request_counter_.record();
        
        // Check if we're exceeding rate limit
        if (request_counter_.count() > max_requests_per_minute_) {
            std::cout << "Rate limit exceeded! " << request_counter_.count() 
                      << " requests in last minute\n";
            return false;  // Reject request
        }
        
        std::cout << "Handling request to " << endpoint 
                  << " (total: " << request_counter_.count() << "/min)\n";
        return true;
    }
    
    void print_stats() {
        std::cout << "Current rate: " << request_counter_.rate_per_second() 
                  << " req/sec (" << request_counter_.count() << " req/min)\n";
    }
};

void example_api_rate_monitoring() {
    std::cout << "\n=== Example 2: API Rate Monitoring ===\n";
    
    APIServer server;
    
    // Simulate API requests
    std::vector<std::string> endpoints = {"/users", "/orders", "/products", "/stats"};
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> endpoint_dist(0, endpoints.size() - 1);
    
    for (int i = 0; i < 15; ++i) {
        std::string endpoint = endpoints[endpoint_dist(gen)];
        server.handle_request(endpoint);
        std::this_thread::sleep_for(100ms);
    }
    
    server.print_stats();
}

// Example 3: Error Rate Monitoring with Alarm
class ErrorMonitor {
private:
    IntervalCounter error_counter_;
    const double alarm_threshold_;  // errors per second
    bool alarm_active_ = false;
    
public:
    ErrorMonitor(double threshold) 
        : error_counter_(300s, 1s), alarm_threshold_(threshold) {}  // 5-minute window
    
    void log_error(const std::string& error_msg) {
        error_counter_.record();
        
        double current_rate = error_counter_.rate_per_second();
        std::cout << "ERROR: " << error_msg 
                  << " (rate: " << std::fixed << std::setprecision(3) 
                  << current_rate << " errors/sec)\n";
        
        // Trigger alarm if threshold exceeded
        if (!alarm_active_ && current_rate > alarm_threshold_) {
            alarm_active_ = true;
            std::cout << "🚨 ALARM TRIGGERED: Error rate exceeded " 
                      << alarm_threshold_ << " errors/sec!\n";
        } else if (alarm_active_ && current_rate <= alarm_threshold_ * 0.8) {
            alarm_active_ = false;
            std::cout << "✅ ALARM CLEARED: Error rate back to normal\n";
        }
    }
    
    void print_status() {
        std::cout << "Error status: " << error_counter_.count() 
                  << " errors in last 5min, rate: " 
                  << error_counter_.rate_per_second() << " errors/sec"
                  << (alarm_active_ ? " 🚨 ALARM ACTIVE" : "") << std::endl;
    }
};

void example_error_monitoring() {
    std::cout << "\n=== Example 3: Error Rate Monitoring ===\n";
    
    ErrorMonitor monitor(0.5);  // Alarm if > 0.5 errors/sec
    
    // Simulate normal operation
    monitor.log_error("Connection timeout");
    std::this_thread::sleep_for(2s);
    
    monitor.log_error("Database error");
    std::this_thread::sleep_for(1s);
    
    // Simulate error burst
    std::cout << "\nSimulating error burst...\n";
    for (int i = 0; i < 5; ++i) {
        monitor.log_error("Service unavailable");
        std::this_thread::sleep_for(500ms);
    }
    
    monitor.print_status();
    
    // Wait and check if alarm clears
    std::cout << "\nWaiting for error rate to decrease...\n";
    std::this_thread::sleep_for(10s);
    monitor.print_status();
}

// Example 4: High-Performance Single-Threaded Variant
void example_high_performance() {
    std::cout << "\n=== Example 4: High-Performance Single-Threaded ===\n";
    
    IntervalCounterST fast_counter(10s, 100ms);  // High resolution: 100ms buckets
    
    auto start = std::chrono::steady_clock::now();
    
    // Simulate high-frequency events
    for (int i = 0; i < 10000; ++i) {
        fast_counter.record();
        if (i % 1000 == 0) {
            std::this_thread::sleep_for(10ms);  // Small delays
        }
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Recorded 10,000 events in " << duration.count() << " microseconds\n";
    std::cout << "Current count: " << fast_counter.count() << std::endl;
    std::cout << "Rate: " << fast_counter.rate_per_second() << " events/sec\n";
    
    // Show bucket breakdown
    auto buckets = fast_counter.bucket_counts();
    std::cout << "Active buckets: " << buckets.size() << std::endl;
}

// Example 5: Network Packet Drop Monitoring
class NetworkMonitor {
private:
    IntervalCounter packet_counter_;
    IntervalCounter drop_counter_;
    
public:
    NetworkMonitor() : packet_counter_(30s), drop_counter_(30s) {}
    
    void process_packet() {
        packet_counter_.record();
    }
    
    void drop_packet(const std::string& reason) {
        drop_counter_.record();
        
        double drop_rate = static_cast<double>(drop_counter_.count()) / 
                          std::max(1UL, packet_counter_.count()) * 100.0;
        
        if (drop_rate > 5.0) {  // Alert if drop rate > 5%
            std::cout << "⚠️  HIGH DROP RATE: " << std::fixed << std::setprecision(1)
                      << drop_rate << "% (" << reason << ")\n";
        }
    }
    
    void print_stats() {
        size_t total_packets = packet_counter_.count();
        size_t dropped_packets = drop_counter_.count();
        double drop_percentage = total_packets > 0 ? 
            static_cast<double>(dropped_packets) / total_packets * 100.0 : 0.0;
        
        std::cout << "Network stats (last 30s):\n";
        std::cout << "  Packets: " << total_packets << std::endl;
        std::cout << "  Dropped: " << dropped_packets 
                  << " (" << std::fixed << std::setprecision(2) 
                  << drop_percentage << "%)\n";
        std::cout << "  Packet rate: " << packet_counter_.rate_per_second() << " pkt/sec\n";
    }
};

void example_network_monitoring() {
    std::cout << "\n=== Example 5: Network Packet Monitoring ===\n";
    
    NetworkMonitor monitor;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> drop_chance(1, 100);
    
    // Simulate network traffic
    for (int i = 0; i < 200; ++i) {
        monitor.process_packet();
        
        // Simulate occasional drops
        if (drop_chance(gen) <= 8) {  // 8% drop chance
            std::vector<std::string> reasons = {
                "Buffer full", "Checksum error", "Timeout", "Network congestion"
            };
            monitor.drop_packet(reasons[gen() % reasons.size()]);
        }
        
        // Vary the timing
        if (i % 20 == 0) {
            std::this_thread::sleep_for(100ms);
        }
    }
    
    monitor.print_stats();
}

// Example 6: Real-time Metrics Dashboard
void example_metrics_dashboard() {
    std::cout << "\n=== Example 6: Real-time Metrics Dashboard ===\n";
    
    IntervalCounter cpu_spikes(60s, 1s);      // CPU spike events
    IntervalCounter memory_warnings(300s);    // Memory warnings (5min window)
    IntervalCounter disk_errors(3600s);       // Disk errors (1 hour window)
    
    // Simulate system events over time
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> event_type(1, 100);
    
    for (int minute = 0; minute < 10; ++minute) {
        std::cout << "\n--- Minute " << minute + 1 << " ---\n";
        
        // Generate events based on probabilities
        for (int second = 0; second < 60; ++second) {
            int roll = event_type(gen);
            
            if (roll <= 15) {  // 15% chance of CPU spike
                cpu_spikes.record();
            }
            if (roll <= 5) {   // 5% chance of memory warning
                memory_warnings.record();
            }
            if (roll <= 1) {   // 1% chance of disk error
                disk_errors.record();
            }
        }
        
        // Print dashboard
        std::cout << "System Health Dashboard:\n";
        std::cout << "  CPU spikes (1min): " << cpu_spikes.count() 
                  << " (" << cpu_spikes.rate_per_second() << "/sec)\n";
        std::cout << "  Memory warnings (5min): " << memory_warnings.count() 
                  << " (" << memory_warnings.rate_per_second() << "/sec)\n";
        std::cout << "  Disk errors (1hr): " << disk_errors.count() 
                  << " (" << disk_errors.rate_per_second() << "/sec)\n";
        
        std::this_thread::sleep_for(1s);  // Simulate real-time updates
    }
}

int main() {
    std::cout << "IntervalCounter / RateTracker Examples\n";
    std::cout << "=====================================\n";
    
    try {
        example_basic_usage();
        example_api_rate_monitoring();
        example_error_monitoring();
        example_high_performance();
        example_network_monitoring();
        example_metrics_dashboard();
        
        std::cout << "\n✅ All examples completed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
