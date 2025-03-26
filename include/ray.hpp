#pragma once
#include "vec.hpp"

struct Ray {
    Vec o, d;
    Ray(Vec o_, Vec d_) : o(o_), d(d_) {}
};