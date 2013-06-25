#include "timer.hpp"

_measurecc::Timer t1,t2;

namespace {


    void foo() {
        t1.start();
        t1.stop();
        t1.total_time();
    }
}

