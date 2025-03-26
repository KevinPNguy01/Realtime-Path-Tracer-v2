#pragma once
#include "sphere.hpp"

Sphere::Sphere(double rad_, Vec p_, Vec e_, const BRDF& brdf_)
    : Shape(brdf_, e_), rad(rad_), p(p_) {}

double Sphere::intersect(const Ray& r, Vec* point, Vec* normal) const { // returns distance, 0 if nohit
    Vec op = p - r.o; // Solve t^2*d.d + 2*t*(o-p).d + (o-p).(o-p)-R^2 = 0
    double b = op.dot(r.d), det = b * b - op.dot(op) + rad * rad;
    if (det < 0) return 0;
    det = sqrt(det);

    double t = 0, t1 = b - det, t2 = b + det;
    double eps = 1e-4;
    if (t2 > eps) t = t2;
    if (t1 > eps) t = t1;
    if (t && point && normal) {
        *point = r.o + r.d * t;
        *normal = (*point - p).normalize();
    }
    return t;
}

void Sphere::sample(Vec& point, Vec& normal, double& pdf) const {
    double xi1 = rng();
    double xi2 = rng();
    double z = 2 * xi1 - 1;
    double x = sqrt(1 - z * z) * cos(2 * PI * xi2);
    double y = sqrt(1 - z * z) * sin(2 * PI * xi2);
    point = p + Vec(x, y, z) * rad;
    normal = (point - p).normalize();
    pdf = 1.0 / (4 * PI * rad * rad);
}