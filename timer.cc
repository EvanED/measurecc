#include "timer.hpp"

void _measurecc_dummy() {
    _measurecc::Timer t("a");
    t.start();
    t.stop();
    t.total_time();
    _measurecc::output_counter(0, "x");
}

