#pragma once
#include "triangle.hpp"

Triangle::Triangle(Vec v0, Vec v1, Vec v2, Vec e, const BRDF& brdf)
    : Shape(brdf, e), v0(v0), v1(v1), v2(v2) {
    Vec edge1 = v1 - v0;
    Vec edge2 = v2 - v0;
    n = edge1.cross(edge2).normalize();
}

double Triangle::intersect(const Ray& ray, Vec* point, Vec* normal) const {
    const Vec edge1 = v1 - v0;
    const Vec edge2 = v2 - v0;
    const Vec h = ray.d.cross(edge2);
    const double a = edge1.dot(h);

    if (std::abs(a) < 1e-8) return 0;

    const double f = 1.0 / a;
    const Vec s = ray.o - v0;
    const double u = f * s.dot(h);
    if (u < 0.0 || u > 1.0) return 0;

    const Vec q = s.cross(edge1);
    const double v = f * ray.d.dot(q);
    if (v < 0.0 || u + v > 1.0) return 0;

    const double t = f * edge2.dot(q);
    if (t <= 1e-8) return 0;

    if (point && normal) {
        *point = ray.o + ray.d * t;
        *normal = n;
    }
    return t;
}

void Triangle::sample(Vec& point, Vec& normal, double& pdf) const {
    double r1 = rng();
    double r2 = rng();

    double sqrt_r1 = sqrt(r1);
    double u = 1 - sqrt_r1;
    double v = sqrt_r1 * (1 - r2);
    double w = sqrt_r1 * r2;

    point = v0 * u + v1 * v + v2 * w;
    normal = n;

    double area = 0.5 * ((v1 - v0).cross(v2 - v0)).length();
    pdf = 1.0 / area;
}

double Triangle::area() const {
    return 0.5 * ((v1 - v0).length() * (v2 - v0).length());
}