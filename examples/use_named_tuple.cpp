#include "named_tuple.h"
#include <iostream>
#include <string>

// --- Define fields for a Person inside a namespace ---
namespace person_fields {
    DEFINE_NAMED_TUPLE_FIELD(ID, int);
    DEFINE_NAMED_TUPLE_FIELD(FirstName, std::string);
    DEFINE_NAMED_TUPLE_FIELD(LastName, std::string);
}

// --- Define fields for a Product inside a different namespace ---
namespace product_fields {
    DEFINE_NAMED_TUPLE_FIELD(ID, long int); // Same field name "ID", but different type
    DEFINE_NAMED_TUPLE_FIELD(ProductName, std::string);
    DEFINE_NAMED_TUPLE_FIELD(Price, double);
}

// --- Create the tuple aliases using the namespaced fields ---
using Person = cpp_utils::named_tuple<true,
    person_fields::ID,
    person_fields::FirstName,
    person_fields::LastName>;

using Product = cpp_utils::named_tuple<true,
    product_fields::ID,
    product_fields::ProductName,
    product_fields::Price>;

int main() {
    // Creating a Person.
    // The type `person_fields::ID` is used for lookup.
    Person p(101, "Alex", "Chen");
    std::cout << "Person ID: " << p.get<person_fields::ID>() << std::endl;

    // Creating a Product.
    // The type `product_fields::ID` is used for lookup.
    Product prd(987654321L, "Super Gadget", 299.99);
    std::cout << "Product ID: " << prd.get<product_fields::ID>() << std::endl;

    // Modifying the product's price
    prd.get<product_fields::Price>() = 249.99;
    std::cout << "New Product Price: $" << prd.get<product_fields::Price>() << std::endl;

    return 0;
}
