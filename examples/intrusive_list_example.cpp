#include "intrusive_list.h"
#include <iostream>

struct MyObject {
    int data;
    cpp_utils::intrusive_list_hook hook;

    MyObject(int d) : data(d) {}
};

int main() {
    MyObject obj1(1);
    MyObject obj2(2);
    MyObject obj3(3);

    cpp_utils::intrusive_list<MyObject, &MyObject::hook> list;

    list.push_back(obj1);
    list.push_back(obj2);
    list.push_front(obj3);

    std::cout << "List size: " << list.size() << std::endl;

    std::cout << "List contents:";
    for (const auto& obj : list) {
        std::cout << " " << obj.data;
    }
    std::cout << std::endl;

    list.pop_front();
    list.pop_back();

    std::cout << "List contents after popping:";
    for (const auto& obj : list) {
        std::cout << " " << obj.data;
    }
    std::cout << std::endl;

    return 0;
}
