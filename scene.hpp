#pragma once
#include "brdf.hpp"
#include "sphere.hpp"

/*
 * Scene configuration
 */
extern const DiffuseBRDF leftWall, rightWall, otherWall, blackSurf, brightSurf;
extern const SpecularBRDF shinySurf;

// Scene: list of spheres
extern const Sphere spheres[];

bool intersect(const Ray& r, double& t, int& id);