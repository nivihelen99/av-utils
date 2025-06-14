#include "include/named_struct.h" // Assuming named_struct.h is in ./include/

// This should be the simplest possible instantiation
NAMED_STRUCT(MyStruct,
    FIELD("value", int)
);

int main() {
    MyStruct m{10};
    (void)m; // Use m to prevent unused variable warning
    return 0;
}
