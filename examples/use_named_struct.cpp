

#include "named_struct.h"


// Fully mutable (like Python dataclass with default settings)
NAMED_STRUCT(Point, 
    FIELD("x", int), 
    FIELD("y", int)
);

// Mixed mutability
NAMED_STRUCT(Person, 
    IMMUTABLE_FIELD("id", int),        // Cannot be changed after creation
    MUTABLE_FIELD("name", std::string), // Can be changed
    MUTABLE_FIELD("age", int)          // Can be changed
);

// Fully immutable (like Python namedtuple)
NAMED_STRUCT(ImmutablePoint, 
    IMMUTABLE_FIELD("x", int), 
    IMMUTABLE_FIELD("y", int)
);

int main() {
    // Mutable structs
    Point p{10, 20};
    p.get<0>() = 30;              // Direct modification
    p.set<"y">(40);               // Using setter
    std::cout << "Modified point: " << p << std::endl; // {30, 40}
    
    // Mixed mutability
    Person john{1, "John Doe", 30};
    john.set<"name">("Jane Doe");  // OK - mutable field
    john.set<"age">(31);           // OK - mutable field
    // john.set<"id">(2);          // ERROR - immutable field
    
    // Check mutability at compile time
    static_assert(Person::is_mutable<"name">());  // true
    static_assert(!Person::is_mutable<"id">());   // false
    
    // Immutable structs
    ImmutablePoint ip{100, 200};
    // ip.get<0>() = 300;          // ERROR - const value
    // ip.set<"x">(300);           // ERROR - immutable field
    
    // But you can still read values
    std::cout << "Immutable point: " << ip << std::endl;
    auto [x, y] = ip;  // Structured binding still works
    
    // DataClass-like behavior
    Point p1{1, 2};
    Point p2{1, 2};
    Point p3{3, 4};
    
    std::cout << "p1 == p2: " << (p1 == p2) << std::endl; // true
    std::cout << "p1 == p3: " << (p1 == p3) << std::endl; // false
    std::cout << "p1 < p3: " << (p1 < p3) << std::endl;   // true
    
    return 0;
}

