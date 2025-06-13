#include "named_struct.h"
#include <string>
#include <iostream>

NAMED_STRUCT(ImmutablePoint,
    IMMUTABLE_FIELD("x", int),
    IMMUTABLE_FIELD("y", int)
);

NAMED_STRUCT(MixedMutabilityPoint,
    FIELD("a", int), // Mutable
    IMMUTABLE_FIELD("b", double) // Immutable
);

int main() {
    // Test const NamedStruct with immutable fields
    const ImmutablePoint ip{100, 200};
    // auto [x, y] = ip.as_tuple();
    auto temp_ip_tuple = ip.as_tuple();
    auto [x, y] = temp_ip_tuple;

    std::cout << "ip: x=" << x << ", y=" << y << std::endl;
    (void)x; (void)y;


    // Test const NamedStruct with mixed mutability fields
    const MixedMutabilityPoint mmp{300, 400.5};
    // auto [a, b_const] = mmp.as_tuple();
    auto temp_mmp_tuple = mmp.as_tuple();
    auto [a, b_const] = temp_mmp_tuple;

    std::cout << "mmp: a=" << a << ", b_const=" << b_const << std::endl;
    (void)a; (void)b_const;

    // Test non-const NamedStruct with mixed mutability fields
    MixedMutabilityPoint mmp_non_const{500, 600.5};
    // auto&& [a_ref, b_const_ref] = mmp_non_const.as_tuple();
    auto temp_mmp_non_const_tuple = mmp_non_const.as_tuple();
    auto& [a_ref, b_const_ref] = temp_mmp_non_const_tuple;

    a_ref = 505; // Should compile, a_ref is int& bound to the original member
    // b_const_ref = 601.5; // Should fail to compile, as b_const_ref should be const double&

    std::cout << "mmp_non_const: a=" << mmp_non_const.get<"a">()
              << ", b_const=" << mmp_non_const.get<"b">() << std::endl;
    (void)b_const_ref; // prevent unused warning


    return 0;
}
