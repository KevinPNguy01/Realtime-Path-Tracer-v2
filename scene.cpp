#pragma once
#include "scene.hpp"

/*
 * Scene configuration
 */

 // Pre-defined BRDFs
const DiffuseBRDF leftWall(Vec(.75, .25, .25)),
rightWall(Vec(.25, .25, .75)),
otherWall(Vec(.75, .75, .75)),
blackSurf(Vec(0.0, 0.0, 0.0)),
brightSurf(Vec(0.9, 0.9, 0.9));
const SpecularBRDF shinySurf(Vec(0.999, 0.999, 0.999));

// Scene: list of spheres
const Sphere spheres[] = {
    Sphere(1e5,  Vec(1e5 + 1,40.8,81.6),   Vec(),         leftWall),   // Left
    Sphere(1e5,  Vec(-1e5 + 99,40.8,81.6), Vec(),         rightWall),  // Right
    Sphere(1e5,  Vec(50,40.8, 1e5),      Vec(),         otherWall),  // Back
    Sphere(1e5,  Vec(50, 1e5, 81.6),     Vec(),         otherWall),  // Bottom
    Sphere(1e5,  Vec(50,-1e5 + 81.6,81.6), Vec(),         otherWall),  // Top
    Sphere(16.5, Vec(27,16.5,47),        Vec(),         brightSurf), // Ball 1
    Sphere(16.5, Vec(73,16.5,78),        Vec(),         shinySurf), // Ball 2
    Sphere(5,  Vec(50,70.0,81.6),      Vec(50,50,50), blackSurf)   // Light
};

bool intersect(const Ray& r, double& t, int& id) {
    double n = sizeof(spheres) / sizeof(Sphere), d, inf = t = 1e20;
    for (int i = int(n); i--;) if ((d = spheres[i].intersect(r)) && d < t) { t = d; id = i; }
    return t < inf;
}