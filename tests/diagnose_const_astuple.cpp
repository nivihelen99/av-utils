#include "named_struct.h" // Adjust include path if necessary from project root
#include <iostream>      // For std::cout
#include <string>        // For std::string if any field were a string

// Define a NamedStruct with mixed mutability, including at least one immutable field.
NAMED_STRUCT(DiagnosePoint,
    FIELD("regular_field", int),         // Mutable by default
    IMMUTABLE_FIELD("const_field", int)  // Immutable
);

int main() {
    const DiagnosePoint dp_instance{10, 20};

    // Call as_tuple() on the const instance.
    // The previous subtask reported a static_assert failure related to
    // a similar line involving 'mmp.as_tuple()'.
    auto result_tuple = dp_instance.as_tuple();

    // Prevent unused variable warning for 'result_tuple'.
    // If this point is reached, the as_tuple() call itself did not trigger
    // the static_assert related to an empty Fields... pack.
    (void)result_tuple;
    // std::cout << "Tuple size: " << std::tuple_size<decltype(result_tuple)>::value << std::endl;

    // If we reach here, the specific call to as_tuple on a const instance
    // did not directly cause the static_assert.
    // This would mean the error is more context-sensitive.
    // If it fails with static_assert, it confirms a deep issue.
    // std::cout << "Minimal const as_tuple() call succeeded without static_assert." << std::endl;

    return 0;
}
