#ifndef EVAN_TIMER_DETAILS_BITS_HPP
#define EVAN_TIMER_DETAILS_BITS_HPP

#include <time.h>

namespace _measurecc {
    namespace details {
        static const long long NANO = 1000000000;

        long long
        now() {
            timespec ts;
            int err = clock_gettime(CLOCK_MONOTONIC, &ts);
            assert(err == 0);
            return (static_cast<long long>(ts.tv_sec) * NANO)
                + ts.tv_nsec;
        }

        double
        to_sec(long long ticks) {
            return static_cast<double>(ticks)/NANO;
        }
    }
}

#endif
