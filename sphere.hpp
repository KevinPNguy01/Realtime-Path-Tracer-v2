#pragma once
#include "shape.hpp"
#include "brdf.hpp"
#include "ray.hpp"
#include "vec.hpp"
#include "util.hpp"

struct Sphere : public Shape {
    Vec p;           // position, emitted radiance
    double rad;         // radius

    Sphere(double rad_, Vec p_, Vec e_, const BRDF& brdf_);

    double intersect(const Ray& r, Vec* point, Vec* normal) const override;

    void sample(Vec& point, Vec& normal, double& pdf) const override;
};