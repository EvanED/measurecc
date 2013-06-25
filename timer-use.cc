#include "timer.hpp"

void foo(_measurecc::Timer t) {
    t.start();
    t.stop();
    t.total_time();
}
