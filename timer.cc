#include "timer.hpp"

namespace {
    _measurecc::Timer t;

    void foo() {
        t.start();
        t.stop();
        t.total_time();
    }
}

