#include "named_struct.h"
#include <string>
#include <iostream>

// Test for StringLiteral as NTTP
template <StringLiteral S>
struct TestStringLiteralNTTP {
    static constexpr auto value_sl = S; // Use a different name to avoid conflict
};


NAMED_STRUCT(ImmutablePoint,
    IMMUTABLE_FIELD("x", int),
    IMMUTABLE_FIELD("y", int)
);

int main() {
    // Test if StringLiteral can be used as NTTP
    // If this line compiles, StringLiteral is likely a structural type.
    [[maybe_unused]] TestStringLiteralNTTP<"test"> test_sl_instance;

    const ImmutablePoint ip{100, 200};
    auto [x, y] = ip;

    (void)x;
    (void)y;
    // std::cout << "ImmutablePoint: x=" << x << ", y=" << y << std::endl;

    return 0;
}
