#pragma once

#include <chrono>
#include <string_view>
#include <stdexcept>
#include <charconv>
#include <algorithm>
#include <vector>
#include <unordered_map>

namespace duration_parser {

/**
 * @brief Exception thrown when duration parsing fails
 */
class parse_error : public std::invalid_argument {
public:
    explicit parse_error(const std::string& msg) : std::invalid_argument(msg) {}
};

namespace detail {
    /**
     * @brief Represents a parsed duration component (value + unit)
     */
    struct parsed_unit {
        double value;
        std::string_view unit;
    };

    /**
     * @brief Unit conversion table mapping unit suffixes to nanosecond multipliers
     */
    inline const std::unordered_map<std::string_view, double>& get_unit_map() {
        static const std::unordered_map<std::string_view, double> units = {
            {"ns", 1.0},                    // nanoseconds
            {"us", 1000.0},                 // microseconds  
            {"ms", 1000000.0},              // milliseconds
            {"s",  1000000000.0},           // seconds
            {"m",  60000000000.0},          // minutes
            {"h",  3600000000000.0}         // hours
        };
        return units;
    }

    /**
     * @brief Skip whitespace characters in string_view
     */
    constexpr std::string_view skip_whitespace(std::string_view sv) {
        auto it = std::find_if_not(sv.begin(), sv.end(), 
            [](char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; });
        return sv.substr(it - sv.begin());
    }

    /**
     * @brief Parse a floating-point number from the beginning of string_view
     * @return pair of (parsed_value, remaining_string)
     */
    std::pair<double, std::string_view> parse_number(std::string_view sv) {
        sv = skip_whitespace(sv);
        if (sv.empty()) {
            throw parse_error("Expected number but found end of string");
        }

        // Find the end of the numeric part
        size_t num_end = 0;
        bool has_dot = false;
        bool has_digit = false;

        // Handle optional leading sign
        if (num_end < sv.size() && (sv[num_end] == '+' || sv[num_end] == '-')) {
            ++num_end;
        }

        // Parse digits and optional decimal point
        while (num_end < sv.size()) {
            char c = sv[num_end];
            if (c >= '0' && c <= '9') {
                has_digit = true;
                ++num_end;
            } else if (c == '.' && !has_dot) {
                has_dot = true;
                ++num_end;
            } else {
                break;
            }
        }

        if (!has_digit) {
            throw parse_error("Invalid number format");
        }

        // Extract the numeric substring
        std::string_view num_str = sv.substr(0, num_end);
        
        // Parse the number (manual parsing since std::from_chars doesn't handle doubles reliably across all compilers)
        double value = 0.0;
        bool negative = false;
        size_t i = 0;
        
        if (i < num_str.size() && num_str[i] == '-') {
            negative = true;
            ++i;
        } else if (i < num_str.size() && num_str[i] == '+') {
            ++i;
        }
        
        // Parse integer part
        while (i < num_str.size() && num_str[i] >= '0' && num_str[i] <= '9') {
            value = value * 10.0 + (num_str[i] - '0');
            ++i;
        }
        
        // Parse fractional part
        if (i < num_str.size() && num_str[i] == '.') {
            ++i;
            double fraction = 0.0;
            double divisor = 10.0;
            while (i < num_str.size() && num_str[i] >= '0' && num_str[i] <= '9') {
                fraction += (num_str[i] - '0') / divisor;
                divisor *= 10.0;
                ++i;
            }
            value += fraction;
        }
        
        if (negative) {
            value = -value;
        }

        return {value, sv.substr(num_end)};
    }

    /**
     * @brief Parse a unit suffix from the beginning of string_view
     * @return pair of (unit_string, remaining_string)
     */
    std::pair<std::string_view, std::string_view> parse_unit(std::string_view sv) {
        sv = skip_whitespace(sv);
        if (sv.empty()) {
            throw parse_error("Expected unit suffix but found end of string");
        }

        // Try to match units in order of length (longest first to handle "ms" before "m")
        const auto& units = get_unit_map();
        std::vector<std::string_view> sorted_units;
        
        for (const auto& [unit, _] : units) {
            sorted_units.push_back(unit);
        }
        
        // Sort by length (descending) to match longest units first
        std::sort(sorted_units.begin(), sorted_units.end(), 
            [](std::string_view a, std::string_view b) {
                return a.length() > b.length();
            });

        for (const auto& unit : sorted_units) {
            if (sv.substr(0, unit.length()) == unit) {
                return {unit, sv.substr(unit.length())};
            }
        }

        // Find the end of the unit (non-digit, non-whitespace characters)
        size_t unit_end = 0;
        while (unit_end < sv.size() && 
               sv[unit_end] != ' ' && sv[unit_end] != '\t' && 
               sv[unit_end] != '\n' && sv[unit_end] != '\r' &&
               (sv[unit_end] < '0' || sv[unit_end] > '9')) {
            ++unit_end;
        }

        if (unit_end == 0) {
            throw parse_error("Expected unit suffix");
        }

        std::string_view unknown_unit = sv.substr(0, unit_end);
        throw parse_error("Unknown unit: '" + std::string(unknown_unit) + "'");
    }

    /**
     * @brief Tokenize a duration string into number-unit pairs
     */
    std::vector<parsed_unit> tokenize(std::string_view input) {
        std::vector<parsed_unit> result;
        
        input = skip_whitespace(input);
        if (input.empty()) {
            throw parse_error("Empty duration string");
        }

        while (!input.empty()) {
            input = skip_whitespace(input);
            if (input.empty()) break;

            auto [value, after_number] = parse_number(input);
            auto [unit, remaining] = parse_unit(after_number);
            
            result.push_back({value, unit});
            input = remaining;
        }

        return result;
    }

    /**
     * @brief Convert parsed units to nanoseconds
     */
    double to_nanoseconds(const std::vector<parsed_unit>& units) {
        const auto& unit_map = get_unit_map();
        double total_ns = 0.0;

        for (const auto& [value, unit] : units) {
            auto it = unit_map.find(unit);
            if (it == unit_map.end()) {
                throw parse_error("Unknown unit: '" + std::string(unit) + "'");
            }
            total_ns += value * it->second;
        }

        return total_ns;
    }
}

/**
 * @brief Parse a human-readable duration string into a std::chrono::duration
 * 
 * Supported formats:
 * - "1h" (1 hour)
 * - "30m" (30 minutes)  
 * - "15s" (15 seconds)
 * - "500ms" (500 milliseconds)
 * - "2h10m5s" (2 hours, 10 minutes, 5 seconds)
 * - "1.5h" (1.5 hours)
 * - "2.25s" (2.25 seconds)
 * 
 * Supported units: ns, us, ms, s, m, h
 * 
 * @tparam DurationOut Target std::chrono::duration type
 * @param input Human-readable duration string
 * @return Parsed duration in the specified type
 * @throws parse_error if parsing fails
 */
template <typename DurationOut>
DurationOut parse_duration(std::string_view input) {
    static_assert(std::is_same_v<DurationOut, std::chrono::duration<typename DurationOut::rep, typename DurationOut::period>>,
                  "DurationOut must be a std::chrono::duration type");

    try {
        auto units = detail::tokenize(input);
        double total_ns = detail::to_nanoseconds(units);
        
        // Convert from nanoseconds to target duration type
        auto ns_duration = std::chrono::nanoseconds(static_cast<std::chrono::nanoseconds::rep>(total_ns));
        return std::chrono::duration_cast<DurationOut>(ns_duration);
    } catch (const parse_error&) {
        throw; // Re-throw our custom exceptions
    } catch (const std::exception& e) {
        throw parse_error("Duration parsing failed: " + std::string(e.what()));
    }
}

/**
 * @brief Parse a duration string with a default fallback value
 * 
 * @tparam DurationOut Target std::chrono::duration type
 * @param input Human-readable duration string
 * @param default_value Value to return if parsing fails
 * @return Parsed duration or default value
 */
template <typename DurationOut>
DurationOut parse_duration_or_default(std::string_view input, DurationOut default_value) noexcept {
    try {
        return parse_duration<DurationOut>(input);
    } catch (...) {
        return default_value;
    }
}

} // namespace duration_parser
