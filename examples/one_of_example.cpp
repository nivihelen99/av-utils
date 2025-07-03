#include "one_of.h" // Adjust path if necessary
#include <iostream>
#include <string>
#include <vector>

// Define a few structs/classes to use with OneOf
struct MyInt {
    int value;
    MyInt(int v = 0) : value(v) { std::cout << "MyInt(" << value << ") constructed\n"; }
    ~MyInt() { std::cout << "MyInt(" << value << ") destructed\n"; }
    MyInt(const MyInt& other) : value(other.value) { std::cout << "MyInt(" << value << ") copy constructed\n"; }
    MyInt(MyInt&& other) noexcept : value(other.value) { other.value = 0; std::cout << "MyInt(" << value << ") move constructed\n"; }
    MyInt& operator=(const MyInt& other) { value = other.value; std::cout << "MyInt(" << value << ") copy assigned\n"; return *this; }
    MyInt& operator=(MyInt&& other) noexcept { value = other.value; other.value = 0; std::cout << "MyInt(" << value << ") move assigned\n"; return *this; }
    void print() const { std::cout << "MyInt value: " << value << "\n"; }
};

struct MyFloat {
    float value;
    MyFloat(float v = 0.0f) : value(v) { std::cout << "MyFloat(" << value << ") constructed\n"; }
    ~MyFloat() { std::cout << "MyFloat(" << value << ") destructed\n"; }
    void print() const { std::cout << "MyFloat value: " << value << "\n"; }
};

struct MyString {
    std::string value;
    MyString(const char* v = "") : value(v) { std::cout << "MyString(\"" << value << "\") constructed\n"; }
    MyString(const std::string& v) : value(v) { std::cout << "MyString(\"" << value << "\") constructed\n"; }
    ~MyString() { std::cout << "MyString(\"" << value << "\") destructed\n"; }
    MyString(const MyString& other) : value(other.value) { std::cout << "MyString(\"" << value << "\") copy constructed\n"; }
    MyString(MyString&& other) noexcept : value(std::move(other.value)) { std::cout << "MyString(\"" << value << "\") move constructed\n"; }
    MyString& operator=(const MyString& other) { value = other.value; std::cout << "MyString(\"" << value << "\") copy assigned\n"; return *this; }
    MyString& operator=(MyString&& other) noexcept { value = std::move(other.value); std::cout << "MyString(\"" << value << "\") move assigned\n"; return *this; }
    void print() const { std::cout << "MyString value: \"" << value << "\"\n"; }
};

// A visitor struct
struct PrintVisitor {
    void operator()(const MyInt& i) const {
        std::cout << "Visitor sees MyInt: ";
        i.print();
    }
    void operator()(const MyFloat& f) const {
        std::cout << "Visitor sees MyFloat: ";
        f.print();
    }
    void operator()(const MyString& s) const {
        std::cout << "Visitor sees MyString: ";
        s.print();
    }
    void operator()(int i) const { // For plain int
        std::cout << "Visitor sees plain int: " << i << "\n";
    }
    void operator()(const std::string& s) const { // For plain std::string
        std::cout << "Visitor sees plain std::string: " << s << "\n";
    }
};

// A visitor that modifies (if it gets a non-const ref)
struct ModifyVisitor {
    void operator()(MyInt& i) const {
        std::cout << "ModifyVisitor changing MyInt from " << i.value;
        i.value *= 2;
        std::cout << " to " << i.value << "\n";
    }
    // const overload, does nothing or prints
    void operator()(const MyInt& i) const {
        std::cout << "ModifyVisitor (const) sees MyInt: " << i.value << "\n";
    }
    void operator()(MyFloat& f) const { // Add other types if needed
        std::cout << "ModifyVisitor changing MyFloat from " << f.value;
        f.value += 1.0f;
        std::cout << " to " << f.value << "\n";
    }
    void operator()(const MyFloat& f) const {
        std::cout << "ModifyVisitor (const) sees MyFloat: " << f.value << "\n";
    }
    void operator()(MyString& s) const {
         std::cout << "ModifyVisitor changing MyString from " << s.value;
        s.value += " (modified)";
        std::cout << " to " << s.value << "\n";
    }
    void operator()(const MyString& s) const {
        std::cout << "ModifyVisitor (const) sees MyString: " << s.value << "\n";
    }
    void operator()(int& i) const {
        std::cout << "ModifyVisitor changing plain int from " << i;
        i *= 3;
        std::cout << " to " << i << "\n";
    }
    void operator()(const int& i) const {
         std::cout << "ModifyVisitor (const) sees plain int: " << i << "\n";
    }
    void operator()(std::string& s) const {
        std::cout << "ModifyVisitor changing plain std::string from " << s;
        s += " (also modified)";
        std::cout << " to " << s << "\n";
    }
     void operator()(const std::string& s) const {
         std::cout << "ModifyVisitor (const) sees plain std::string: " << s << "\n";
    }
};


int main() {
    std::cout << "--- Basic Construction and Introspection ---\n";
    OneOf<MyInt, MyFloat, MyString> var1;
    std::cout << "var1 created. Has value? " << std::boolalpha << var1.has_value() << " (Index: " << var1.index() << ")\n";

    var1.set(MyInt(10));
    std::cout << "var1 set to MyInt. Has value? " << var1.has_value() << " (Index: " << var1.index() << ")\n";
    if (var1.has<MyInt>()) {
        std::cout << "var1 has MyInt. Value: " << var1.get_if<MyInt>()->value << "\n";
        std::cout << "Type from type(): " << var1.type().name() << "\n";
    }

    var1.set(MyFloat(20.5f));
    std::cout << "var1 set to MyFloat. Has value? " << var1.has_value() << " (Index: " << var1.index() << ")\n";
    if (var1.has<MyFloat>()) {
        std::cout << "var1 has MyFloat. Value: " << var1.get_if<MyFloat>()->value << "\n";
    }
    if (var1.has<MyInt>()) { // Should be false
        std::cout << "var1 has MyInt (error if this prints).\n";
    } else {
        std::cout << "var1 does not have MyInt (correct).\n";
    }

    std::cout << "\n--- Visiting ---\n";
    PrintVisitor visitor;
    var1.visit(visitor); // Should visit MyFloat

    var1.emplace<MyString>("Hello OneOf");
    var1.visit(visitor); // Should visit MyString

    const auto& c_var1 = var1;
    std::cout << "Visiting const OneOf:\n";
    c_var1.visit(visitor); // const visit

    std::cout << "\n--- Modifying Visit ---\n";
    ModifyVisitor mod_visitor;
    var1.visit(mod_visitor); // Non-const visit, MyString should be modified
    var1.visit(visitor);     // Print to see modification

    // Test with primitive types
    OneOf<int, std::string, double> var2(42);
    std::cout << "\n--- Primitives Test ---\n";
    var2.visit(visitor);
    var2.visit(mod_visitor);
    var2.visit(visitor);

    var2.set(std::string("Test String"));
    var2.visit(visitor);
    var2.visit(mod_visitor);
    var2.visit(visitor);


    std::cout << "\n--- Copy and Move Semantics ---\n";
    std::cout << "Creating var3 with MyInt(100):\n";
    OneOf<MyInt, MyFloat, MyString> var3(MyInt(100)); // MyInt(100) constructed, MyInt(100) move constructed (into OneOf)
    var3.visit(visitor);

    std::cout << "Copy constructing var4 from var3:\n";
    OneOf<MyInt, MyFloat, MyString> var4 = var3; // MyInt(100) copy constructed (from var3's MyInt to var4's MyInt)
    var3.visit(visitor); // Original still valid
    var4.visit(visitor); // Copy has the value

    std::cout << "Move constructing var5 from var3:\n";
    OneOf<MyInt, MyFloat, MyString> var5 = std::move(var3); // MyInt(100) move constructed (from var3's MyInt to var5's MyInt)
                                                          // var3's MyInt(0) destructed (original MyInt in var3 is now moved-from and then var3 is reset)
    std::cout << "var3 after move: Has value? " << var3.has_value() << " (Index: " << var3.index() << ")\n";
    // var3.visit(visitor); // Should throw or be valueless
    var5.visit(visitor);

    std::cout << "Assigning MyString to var4:\n";
    var4.set(MyString("Another String")); // MyInt(100) destructed (in var4), MyString constructed
    var4.visit(visitor);

    std::cout << "Copy assigning var4 (MyString) to var5 (MyInt):\n";
    // MyInt(100) in var5 destructed
    // MyString in var4 copy constructed into var5
    var5 = var4;
    var4.visit(visitor);
    var5.visit(visitor);

    std::cout << "Move assigning var4 (MyString) to var3 (valueless):\n";
    // MyString in var4 move constructed into var3
    // MyString in var4 is then valueless (moved-from), var4 itself becomes valueless.
    var3 = std::move(var4);
    std::cout << "var4 after move assignment: Has value? " << var4.has_value() << " (Index: " << var4.index() << ")\n";
    var3.visit(visitor);
    // var4.visit(visitor); // Should throw or be valueless


    std::cout << "\n--- Resetting ---\n";
    var1.visit(visitor);
    std::cout << "Resetting var1. Has value before? " << var1.has_value() << "\n";
    var1.reset(); // MyString destructed
    std::cout << "Has value after? " << var1.has_value() << " (Index: " << var1.index() << ")\n";
    try {
        var1.visit(visitor); // Should throw
    } catch (const std::bad_variant_access& e) {
        std::cout << "Caught expected exception: " << e.what() << "\n";
    }
    try {
        std::cout << var1.type().name() << "\n"; // Should throw
    } catch (const std::bad_variant_access& e) {
        std::cout << "Caught expected exception: " << e.what() << "\n";
    }

    std::cout << "\n--- Emplace example ---\n";
    OneOf<MyInt, MyString> var6;
    var6.emplace<MyInt>(777); // MyInt(777) constructed
    var6.visit(visitor);
    var6.emplace<MyString>("Emplaced String"); // MyInt(777) destructed, MyString("Emplaced String") constructed
    var6.visit(visitor);

    std::cout << "\n--- End of Example ---\n";
    // Destructions will happen here as variables go out of scope
    // var6: MyString("Emplaced String") destructed
    // var5: MyString("Another String") destructed
    // var4: (valueless)
    // var3: MyString("Another String") destructed
    // var2: std::string("Test String (also modified)") destructed
    // var1: (valueless)
    return 0;
}
