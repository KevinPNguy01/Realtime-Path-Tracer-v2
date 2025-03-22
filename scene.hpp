#pragma once
#include "brdf.hpp"
#include "shape.hpp"
#include "sphere.hpp"
#include "triangle.hpp"

extern const Shape* shapes[];

bool intersect(const Ray& r, double& t, int& id);