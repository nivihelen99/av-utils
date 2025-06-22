#include "scoped_timer.h"
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <thread>
#include <vector> // For custom callback test data storage

// Helper to check if a duration is within an expected range
// Generous allowance for CI environments and timer/thread overhead.
const long long ALLOWED_FIXED_OVERHEAD_US = 4000; // 4ms fixed overhead, increased for GTest overhead
const double ALLOWED_RELATIVE_ERROR = 0.85; // 85% relative error, generous for CI

// For very short sleep durations, the actual time can be significantly more due to scheduling granularity.
const long long MIN_EXPECTED_DURATION_FOR_ACCURATE_CHECK_US = 5000; // 5ms
const long long ADDITIONAL_TOLERANCE_FOR_SHORT_SLEEPS_US = 7000; // 7ms additional upper tolerance

bool check_duration(long long actual_us, long long expected_us, const std::string& context) {
    long long lower_bound = static_cast<long long>(expected_us * (1.0 - ALLOWED_RELATIVE_ERROR)) - ALLOWED_FIXED_OVERHEAD_US;
    if (lower_bound < 0) lower_bound = 0;

    long long upper_bound = static_cast<long long>(expected_us * (1.0 + ALLOWED_RELATIVE_ERROR)) + ALLOWED_FIXED_OVERHEAD_US;

    if (expected_us < MIN_EXPECTED_DURATION_FOR_ACCURATE_CHECK_US) {
        upper_bound = std::max(upper_bound, expected_us + ADDITIONAL_TOLERANCE_FOR_SHORT_SLEEPS_US);
    }
    // Ensure actual_us is at least positive if expected is positive (or zero)
    // And for very small expected durations, ensure it's at least *some* time, not negative.
    if (expected_us >=0 && actual_us < 0) actual_us = 0;


    bool in_range = actual_us >= lower_bound && actual_us <= upper_bound;
    if (!in_range) {
        std::cerr << "[GTEST_MSG] Duration check failed for " << context << ": Actual = " << actual_us
                  << "µs, Expected = " << expected_us
                  << "µs (Range: [" << lower_bound << ", " << upper_bound << "])\n";
    }
    return in_range;
}

TEST(ScopedTimerTest, TimerAccuracy) {
    std::ostringstream oss;
    long long sleep_duration_ms = 25; // Increased sleep for more stable test
    long long expected_us = sleep_duration_ms * 1000;

    {
        ScopedTimer timer("accuracy_test", oss);
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_duration_ms));
    }

    std::string output = oss.str();
    ASSERT_NE(output.find("[ScopedTimer] accuracy_test: "), std::string::npos);

    size_t duration_pos = output.find(": ") + 2;
    size_t us_pos = output.find(" µs");
    ASSERT_NE(duration_pos, std::string::npos);
    ASSERT_NE(us_pos, std::string::npos);

    long long reported_duration_us = std::stoll(output.substr(duration_pos, us_pos - duration_pos));
    EXPECT_TRUE(check_duration(reported_duration_us, expected_us, "TimerAccuracy"));
}

TEST(ScopedTimerTest, LabelOutput) {
    std::ostringstream oss;
    std::string test_label = "my custom label";
    {
        ScopedTimer timer(test_label, oss);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    std::string output = oss.str();
    EXPECT_NE(output.find(test_label), std::string::npos);
}

TEST(ScopedTimerTest, AnonymousTimer) {
    std::ostringstream oss;
    long long sleep_duration_ms = 5;
    long long expected_us = sleep_duration_ms * 1000;
    {
        ScopedTimer timer("", oss); // Testing the empty label version with a stream
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_duration_ms));
    }
    std::string output = oss.str();
    ASSERT_NE(output.find("[ScopedTimer] : "), std::string::npos); // Empty label

    size_t duration_pos = output.find(": ") + 2;
    size_t us_pos = output.find(" µs");
    ASSERT_NE(duration_pos, std::string::npos);
    ASSERT_NE(us_pos, std::string::npos);
    long long reported_duration_us = std::stoll(output.substr(duration_pos, us_pos - duration_pos));
    EXPECT_TRUE(check_duration(reported_duration_us, expected_us, "AnonymousTimer"));
}

TEST(ScopedTimerTest, CustomStream) {
    std::ostringstream my_stream;
    std::string test_label = "custom_stream_test";
    long long sleep_duration_ms = 10;
    long long expected_us = sleep_duration_ms * 1000;

    {
        ScopedTimer timer(test_label, my_stream);
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_duration_ms));
    }

    std::string output = my_stream.str();
    EXPECT_NE(output.find(test_label), std::string::npos);
    size_t duration_pos = output.find(": ") + 2;
    size_t us_pos = output.find(" µs");
    ASSERT_NE(duration_pos, std::string::npos);
    ASSERT_NE(us_pos, std::string::npos);
    long long reported_duration_us = std::stoll(output.substr(duration_pos, us_pos - duration_pos));
    EXPECT_TRUE(check_duration(reported_duration_us, expected_us, "CustomStream"));
}

struct CallbackData {
    std::string label;
    std::chrono::microseconds duration;
    bool called = false;
};

TEST(ScopedTimerTest, CustomCallback) {
    CallbackData data;
    std::string test_label = "callback_test";
    long long sleep_duration_ms = 15;
    long long expected_us = sleep_duration_ms * 1000;

    auto callback = [&](const std::string& label, std::chrono::microseconds duration) {
        data.label = label;
        data.duration = duration;
        data.called = true;
    };

    {
        ScopedTimer timer(test_label, callback);
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_duration_ms));
    }

    ASSERT_TRUE(data.called);
    EXPECT_EQ(data.label, test_label);
    EXPECT_TRUE(check_duration(data.duration.count(), expected_us, "CustomCallback"));
}

TEST(ScopedTimerTest, ResetFunctionality) {
    std::ostringstream oss;
    std::string test_label = "reset_test";
    long long sleep1_ms = 10;
    long long sleep2_ms = 12;
    long long expected_us_after_reset = sleep2_ms * 1000;

    {
        ScopedTimer timer(test_label, oss);
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep1_ms));
        timer.reset();
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep2_ms));
    }

    std::string output = oss.str();
    EXPECT_NE(output.find(test_label), std::string::npos);
    size_t duration_pos = output.find(": ") + 2;
    size_t us_pos = output.find(" µs");
    ASSERT_NE(duration_pos, std::string::npos);
    ASSERT_NE(us_pos, std::string::npos);
    long long reported_duration_us = std::stoll(output.substr(duration_pos, us_pos - duration_pos));
    EXPECT_TRUE(check_duration(reported_duration_us, expected_us_after_reset, "ResetFunctionality"));
}

TEST(ScopedTimerTest, ElapsedFunctionality) {
    std::ostringstream oss;
    std::string test_label = "elapsed_test";
    long long sleep1_ms = 8;
    long long sleep2_ms = 10;
    long long expected_us_elapsed1 = sleep1_ms * 1000;
    long long expected_us_total = (sleep1_ms + sleep2_ms) * 1000;

    std::chrono::microseconds elapsed1_us, elapsed2_us;

    {
        ScopedTimer timer(test_label, oss);
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep1_ms));
        elapsed1_us = timer.elapsed();
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep2_ms));
        elapsed2_us = timer.elapsed();
    }

    EXPECT_TRUE(check_duration(elapsed1_us.count(), expected_us_elapsed1, "ElapsedFunctionality_Elapsed1"));
    EXPECT_GT(elapsed2_us.count(), elapsed1_us.count());
    EXPECT_TRUE(check_duration(elapsed2_us.count(), expected_us_total, "ElapsedFunctionality_Elapsed2"));

    std::string output = oss.str();
    EXPECT_NE(output.find(test_label), std::string::npos);
    size_t duration_pos = output.find(": ") + 2;
    size_t us_pos = output.find(" µs");
    ASSERT_NE(duration_pos, std::string::npos);
    ASSERT_NE(us_pos, std::string::npos);
    long long reported_duration_us = std::stoll(output.substr(duration_pos, us_pos - duration_pos));
    EXPECT_TRUE(check_duration(reported_duration_us, expected_us_total, "ElapsedFunctionality_FinalReport"));
}

TEST(ScopedTimerTest, MacroScopedTimer) {
    std::ostringstream oss;
    std::string macro_label = "macro_test_label";
    long long sleep_duration_ms = 7;
    long long expected_us = sleep_duration_ms * 1000;

    std::streambuf* old_cout_rdbuf = std::cout.rdbuf();
    std::cout.rdbuf(oss.rdbuf());

    {
        SCOPED_TIMER(macro_label.c_str());
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_duration_ms));
    }

    std::cout.rdbuf(old_cout_rdbuf);

    std::string output = oss.str();
    EXPECT_NE(output.find(macro_label), std::string::npos);
    size_t duration_pos = output.find(": ") + 2;
    size_t us_pos = output.find(" µs");
    ASSERT_NE(duration_pos, std::string::npos);
    ASSERT_NE(us_pos, std::string::npos);
    long long reported_duration_us = std::stoll(output.substr(duration_pos, us_pos - duration_pos));
    EXPECT_TRUE(check_duration(reported_duration_us, expected_us, "MacroScopedTimer"));
}

TEST(ScopedTimerTest, MacroScopedTimerAuto) {
    std::ostringstream oss;
    long long sleep_duration_ms = 6;
    long long expected_us = sleep_duration_ms * 1000;

    std::streambuf* old_cout_rdbuf = std::cout.rdbuf();
    std::cout.rdbuf(oss.rdbuf());

    {
        SCOPED_TIMER_AUTO();
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_duration_ms));
    }

    std::cout.rdbuf(old_cout_rdbuf);

    std::string output = oss.str();
    EXPECT_NE(output.find("[ScopedTimer] : "), std::string::npos);
    size_t duration_pos = output.find(": ") + 2;
    size_t us_pos = output.find(" µs");
    ASSERT_NE(duration_pos, std::string::npos);
    ASSERT_NE(us_pos, std::string::npos);
    long long reported_duration_us = std::stoll(output.substr(duration_pos, us_pos - duration_pos));
    EXPECT_TRUE(check_duration(reported_duration_us, expected_us, "MacroScopedTimerAuto"));
}

// It might be good to add a test for the default constructor ScopedTimer()
// which prints to std::cout and has an empty label.
TEST(ScopedTimerTest, DefaultConstructorPrintsToCout) {
    std::ostringstream oss;
    long long sleep_duration_ms = 4; // Short sleep
    long long expected_us = sleep_duration_ms * 1000;

    std::streambuf* old_cout_rdbuf = std::cout.rdbuf();
    std::cout.rdbuf(oss.rdbuf());
    {
        ScopedTimer timer; // Uses ScopedTimer() -> ScopedTimer("") -> ScopedTimer("", std::cout)
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_duration_ms));
    }
    std::cout.rdbuf(old_cout_rdbuf);

    std::string output = oss.str();
    EXPECT_NE(output.find("[ScopedTimer] : "), std::string::npos); // Empty label part
    size_t duration_pos = output.find(": ") + 2;
    size_t us_pos = output.find(" µs");
    ASSERT_NE(duration_pos, std::string::npos);
    ASSERT_NE(us_pos, std::string::npos);
    long long reported_duration_us = std::stoll(output.substr(duration_pos, us_pos - duration_pos));
    EXPECT_TRUE(check_duration(reported_duration_us, expected_us, "DefaultConstructorToCout"));
}
