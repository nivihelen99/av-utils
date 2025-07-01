#include "../include/magnitude_map.h" // Adjust path if necessary from build location
#include <iostream>
#include <string>
#include <vector>
#include <iomanip> // For std::fixed and std::setprecision

void print_events(const std::string& title, const std::vector<std::pair<double, std::string>>& events) {
    std::cout << title << ":" << std::endl;
    if (events.empty()) {
        std::cout << "  (No events found)" << std::endl;
        return;
    }
    for (const auto& p : events) {
        std::cout << "  Time: " << std::fixed << std::setprecision(2) << p.first
                  << ", Event: \"" << p.second << "\"" << std::endl;
    }
    std::cout << std::endl;
}

void print_sensor_readings(const std::string& title, const std::vector<std::pair<int, int>>& readings) {
    std::cout << title << ":" << std::endl;
    if (readings.empty()) {
        std::cout << "  (No readings found)" << std::endl;
        return;
    }
    for (const auto& p : readings) {
        std::cout << "  ID: " << p.first << ", Value: " << p.second << std::endl;
    }
    std::cout << std::endl;
}

int main() {
    std::cout << "--- MagnitudeMap Example: Time-based Events ---" << std::endl;
    cpp_utils::MagnitudeMap<double, std::string> event_log;

    event_log.insert(10.5, "System Start");
    event_log.insert(12.3, "User Login");
    event_log.insert(12.8, "Data Processing Initiated");
    event_log.insert(15.0, "Warning: High CPU Load");
    event_log.insert(15.2, "Task A Completed");
    event_log.insert(18.9, "User Logout");
    event_log.insert(20.1, "System Shutdown Sequence");

    std::cout << "Total events logged: " << event_log.size() << std::endl;
    std::cout << "Is event log empty? " << (event_log.empty() ? "Yes" : "No") << std::endl << std::endl;

    double time_query1 = 12.5;
    double magnitude1 = 0.5; // Events between 12.0 and 13.0
    auto nearby_events1 = event_log.find_within_magnitude(time_query1, magnitude1);
    print_events("Events near " + std::to_string(time_query1) + " (magnitude " + std::to_string(magnitude1) + ")", nearby_events1);

    double time_query2 = 15.1;
    double magnitude2 = 0.1; // Events between 15.0 and 15.2
    auto nearby_events2 = event_log.find_within_magnitude(time_query2, magnitude2);
    print_events("Events near " + std::to_string(time_query2) + " (magnitude " + std::to_string(magnitude2) + ")", nearby_events2);

    double time_query3 = 5.0;
    double magnitude3 = 1.0; // Events between 4.0 and 6.0 (should be none)
    auto nearby_events3 = event_log.find_within_magnitude(time_query3, magnitude3);
    print_events("Events near " + std::to_string(time_query3) + " (magnitude " + std::to_string(magnitude3) + ")", nearby_events3);

    double time_query4 = 15.0;
    double magnitude4 = 0.0; // Exact match for time 15.0
    auto nearby_events4 = event_log.find_within_magnitude(time_query4, magnitude4);
    print_events("Events at " + std::to_string(time_query4) + " (magnitude " + std::to_string(magnitude4) + ")", nearby_events4);

    std::cout << "\n--- MagnitudeMap Example: Sensor Readings (Integer Keys) ---" << std::endl;
    cpp_utils::MagnitudeMap<int, int> sensor_readings;
    sensor_readings.insert(100, 25);
    sensor_readings.insert(105, 26);
    sensor_readings.insert(110, 24);
    sensor_readings.insert(150, 30);
    sensor_readings.insert(153, 31);
    sensor_readings.insert(160, 29);

    std::cout << "Checking for sensor ID 110: "
              << (sensor_readings.contains(110) ? "Found" : "Not Found") << std::endl;
    if (sensor_readings.contains(110)) {
        std::cout << "Value for sensor ID 110: " << *sensor_readings.get(110) << std::endl;
    }

    sensor_readings.remove(110);
    std::cout << "After removing sensor ID 110, checking again: "
              << (sensor_readings.contains(110) ? "Found" : "Not Found") << std::endl << std::endl;

    int sensor_id_query1 = 152;
    int id_magnitude1 = 3; // Readings for IDs from 149 to 155
    auto nearby_readings1 = sensor_readings.find_within_magnitude(sensor_id_query1, id_magnitude1);
    print_sensor_readings("Sensor readings near ID " + std::to_string(sensor_id_query1) + " (magnitude " + std::to_string(id_magnitude1) + ")", nearby_readings1);

    int sensor_id_query2 = 200;
    int id_magnitude2 = 5; // Should find no readings
    auto nearby_readings2 = sensor_readings.find_within_magnitude(sensor_id_query2, id_magnitude2);
    print_sensor_readings("Sensor readings near ID " + std::to_string(sensor_id_query2) + " (magnitude " + std::to_string(id_magnitude2) + ")", nearby_readings2);

    std::cout << "Example completed." << std::endl;

    return 0;
}
