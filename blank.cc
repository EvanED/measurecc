#include <iostream>
#include <unistd.h>

void f() {
    sleep(1);
}

int main() {
    std::cout << "In main\n";
    f();
    sleep(1);
    f();
}

