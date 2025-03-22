#pragma once
#include "util.hpp"

RNG rng;

RNG::RNG() : distrb(0.0, 1.0), engines() {}

void RNG::init(int nworkers) {
    std::random_device rd;
    engines.resize(nworkers);
    for (int i = 0; i < nworkers; ++i)
        engines[i].seed(rd());
}

double RNG::operator()() {
    int id = omp_get_thread_num();
    return distrb(engines[id]);
}

double clamp(double x) {
    return x < 0 ? 0 : x > 1 ? 1 : x;
}

int toInt(double x) {
    return static_cast<int>(std::pow(clamp(x), 1.0 / 2.2) * 255 + .5);
}


void createLocalCoord(const Vec& n, Vec& u, Vec& v, Vec& w) {
    w = n;
    u = ((std::abs(w.x) > .1 ? Vec(0, 1) : Vec(1)).cross(w)).normalize();
    v = w.cross(u);
}