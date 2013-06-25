#ifndef EVAN_TIMER_HPP
#define EVAN_TIMER_HPP

#include <cassert>
#include <iostream>

#include "details/timer_bits.hpp"

namespace _measurecc {
    class Timer {
    public:
        typedef long long int64;

        int64 _current_start;
        int64 _total_time;
        int _depth;
        const char * _name;
    public:
        Timer(const char * c)
            : _current_start(0)
            , _total_time(0)
            , _depth(0)
            , _name(c)
        {}

        ~Timer() {
            std::cout << "Timer " << _name << ": " << total_time() << "\n";
        }

        void start() {
            assert(_current_start == 0 ^ _depth > 0);
            ++_depth;
            if (_depth == 1) {
                _current_start = details::now();
            }
        }

        void stop() {
            assert(_depth > 0 && _current_start != 0);
            --_depth;
            if (_depth == 0) {
                int64 end = details::now();
                _total_time += (end - _current_start);
                _current_start = 0;
            }
        }

        double total_time() const {
            assert(_current_start == 0 && _depth == 0);
            return details::to_sec(_total_time);
        }
    };
}

#endif

