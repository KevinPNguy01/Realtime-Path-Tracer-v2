#pragma once
#include "scene.hpp"

/*
 * Scene configuration
 */

 // Pre-defined BRDFs
const DiffuseBRDF leftWall(Vec(.75, .25, .25)),
rightWall(Vec(.25, .25, .75)),
otherWall(Vec(.75, .75, .75)),
redSurf(Vec(0.65, 0.05, 0.05)),
blueSurf(Vec(0, 0, 1)),
greenSurf(Vec(0.12, 0.45, 0.15)),
orangeSurf(Vec(.75, .5, .25)),
yellowSurf(Vec(.75, .75, .25)),
cyanSurf(Vec(.16, .45, .56)),
magentaSurf(Vec(.75, .25, .75)),
blackSurf(Vec(0.0, 0.0, 0.0)),
brightSurf(Vec(0.9, 0.9, 0.9));
const SpecularBRDF shinySurf(Vec(0.999, 0.999, 0.999));

// Scene: list of shapes
const Shape* shapes[] = {
    new Triangle(Vec(1.5, 10, -1.5), Vec(1.5, 10, 1.5), Vec(-1.5, 10, 1.5), Vec(150,150,150), blackSurf),   // Light
    new Triangle(Vec(-1.5, 10, -1.5), Vec(1.5, 10, -1.5), Vec(-1.5, 10, 1.5), Vec(150,150,150), blackSurf),   // Light
    new Sphere(1e5,  Vec(1e5 + 5, 0, 0),   Vec(),         redSurf),   // Left
    new Sphere(1e5,  Vec(-1e5 - 5, 0, 0), Vec(),         greenSurf),  // Right
    new Sphere(1e5,  Vec(0, 0, -1e5 - 5),      Vec(),         otherWall),  // Back
    new Sphere(1e5,  Vec(0, -1e5, 0),     Vec(),         otherWall),  // Bottom
    new Sphere(1e5,  Vec(0, 1e5 + 10, 0), Vec(),         otherWall),  // Top
    new STLModel("snorlax.stl", cyanSurf, Vec(0, 3.25, -2), Vec(), true, 7),
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