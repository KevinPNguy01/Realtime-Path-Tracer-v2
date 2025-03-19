#pragma once
#include "brdf.hpp"
#include "ray.hpp"
#include "vec.hpp"
#include "util.hpp"

struct Sphere {
    Vec p, e;           // position, emitted radiance
    double rad;         // radius
    const BRDF& brdf;   // BRDF

    Sphere(double rad_, Vec p_, Vec e_, const BRDF& brdf_) :
        rad(rad_), p(p_), e(e_), brdf(brdf_) {}

    double intersect(const Ray& r) const { // returns distance, 0 if nohit
        Vec op = p - r.o; // Solve t^2*d.d + 2*t*(o-p).d + (o-p).(o-p)-R^2 = 0
        double t, eps = 1e-4, b = op.dot(r.d), det = b * b - op.dot(op) + rad * rad;
        if (det < 0) return 0; else det = sqrt(det);
        return (t = b - det) > eps ? t : ((t = b + det) > eps ? t : 0);
    }

    /**
     * Sample a random point on the sphere
     */
    void sample(Vec& point, double& pdf) const {
        double xi1 = rng();
        double xi2 = rng();
        double z = 2 * xi1 - 1;
        double x = sqrt(1 - z * z) * cos(2 * PI * xi2);
        double y = sqrt(1 - z * z) * sin(2 * PI * xi2);
        point = p + Vec(x, y, z) * rad;
        pdf = 1.0 / (4 * PI * rad * rad);
    }
};