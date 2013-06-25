#include "timer.hpp"

_measurecc::Timer t1,t2;
static int x = 0;

void foo() {
    x=5;
    t1.start();
    t1.stop();
    t1.total_time();
}

