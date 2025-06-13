

#include "named_struct.h"

// Define a Point struct
NAMED_STRUCT(Point, FIELD("x", int), FIELD("y", int));

// Define a Person struct
NAMED_STRUCT(Person, 
    FIELD("name", std::string), 
    FIELD("age", int), 
    FIELD("height", double)
);

int main() {
    // Create instances
    Point p1{10, 20};
    Point p2(30, 40);
    
    Person john{"John Doe", 30, 5.9};
    
    // Access by index
    std::cout << "p1.x = " << p1.get<0>() << std::endl;
    std::cout << "p1.y = " << p1.get<1>() << std::endl;
    
    // Access by name
    std::cout << "john's name = " << john.get<"name">() << std::endl;
    std::cout << "john's age = " << john.get<"age">() << std::endl;
    
    // Structured binding support
    auto [x, y] = p1;
    auto [name, age, height] = john;
    
    // Pretty printing
    std::cout << "Point: " << p1 << std::endl;
    std::cout << "Person: " << john << std::endl;
    
    // JSON serialization
    std::cout << "Point JSON: " << to_json(p1) << std::endl;
    std::cout << "Person JSON: " << to_json(john) << std::endl;
    
    // Comparison
    Point p3{10, 20};
    std::cout << "p1 == p3: " << (p1 == p3) << std::endl;
    
    return 0;
}

