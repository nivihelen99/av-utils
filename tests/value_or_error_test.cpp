#define CATCH_CONFIG_MAIN
#include "value_or_error.h"
#include "include/catch.hpp"
#include <string>

TEST_CASE("value_or_error", "[value_or_error]") {
    SECTION("WithValue") {
        cxxds::value_or_error<int, std::string> voe(42);
        REQUIRE(voe.has_value());
        REQUIRE_FALSE(voe.has_error());
        REQUIRE(voe.value() == 42);
        REQUIRE_THROWS_AS(voe.error(), std::logic_error);
    }

    SECTION("WithError") {
        cxxds::value_or_error<int, std::string> voe("error message");
        REQUIRE_FALSE(voe.has_value());
        REQUIRE(voe.has_error());
        REQUIRE(voe.error() == "error message");
        REQUIRE_THROWS_AS(voe.value(), std::logic_error);
    }

    SECTION("MoveWithValue") {
        cxxds::value_or_error<std::unique_ptr<int>, std::string> voe(std::make_unique<int>(42));
        REQUIRE(voe.has_value());
        auto val = std::move(voe).value();
        REQUIRE(*val == 42);
    }

    SECTION("MoveWithError") {
        cxxds::value_or_error<int, std::unique_ptr<std::string>> voe(std::make_unique<std::string>("error message"));
        REQUIRE(voe.has_error());
        auto err = std::move(voe).error();
        REQUIRE(*err == "error message");
    }
}
