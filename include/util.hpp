#pragma once
#include <random>
#include <omp.h>
#include "vec.hpp"
#define PI 3.1415926535897932384626433832795

/*
 * Thread-safe random number generator
 */

struct RNG {
    RNG();

    void init(int nworkers);

    double operator()();

    std::uniform_real_distribution<double> distrb;
    std::vector<std::mt19937> engines;
};

extern RNG rng;

/*
 * Utility functions
 */

double clamp(double x);
int toInt(double x);
void createLocalCoord(const Vec& n, Vec& u, Vec& v, Vec& w);