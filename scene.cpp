#pragma once
#include "scene.hpp"

/*
 * Scene configuration
 */

 // Pre-defined BRDFs
const DiffuseBRDF leftWall(Vec(.75, .25, .25)),
rightWall(Vec(.25, .25, .75)),
otherWall(Vec(.75, .75, .75)),
greenSurf(Vec(.25, .75, .25)),
orangeSurf(Vec(.75, .5, .25)),
yellowSurf(Vec(.75, .75, .25)),
cyanSurf(Vec(.25, .75, .75)),
magentaSurf(Vec(.75, .25, .75)),
blackSurf(Vec(0.0, 0.0, 0.0)),
brightSurf(Vec(0.9, 0.9, 0.9));
const SpecularBRDF shinySurf(Vec(0.999, 0.999, 0.999));

// Scene: list of shapes
const Shape* shapes[] = {
    new Sphere(0.5, Vec(0, 8, 2),      Vec(40,40,40), blackSurf),   // Light
    new Sphere(1e5,  Vec(1e5 + 5, 0, 0),   Vec(),         leftWall),   // Left
    new Sphere(1e5,  Vec(-1e5 - 5, 0, 0), Vec(),         rightWall),  // Right
    new Sphere(1e5,  Vec(0, 0, -1e5 - 5),      Vec(),         otherWall),  // Back
    new Sphere(1e5,  Vec(0, -1e5, 0),     Vec(),         otherWall),  // Bottom
    new Sphere(1e5,  Vec(0, 1e5 + 10, 0), Vec(),         otherWall),  // Top
    new STLModel("octahedron.stl", greenSurf, Vec(2, 2, 2), Vec(), true, 4),
    new Sphere(2.5,  Vec(-2, 2.5, -2), Vec(), orangeSurf),
};

bool intersect(const Ray& r, double& t, int& id, Vec* point, Vec* normal) {
    double d, inf = t = 1e20;
    int n = sizeof(shapes) / sizeof(void*);
    Vec pos, norm;
    for (int i = 0; i < n; ++i) {
        if ((d = shapes[i]->intersect(r, &pos, &norm)) && d < t) {
            t = d; id = i;
            if (point && normal) {
                *point = pos, * normal = norm;
            }
        }
    }
    return t < inf;
}