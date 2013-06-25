#include "timer.hpp"

void foo() {
    _measurecc::Timer t("a");
    t.start();
    t.stop();
    t.total_time();
}

