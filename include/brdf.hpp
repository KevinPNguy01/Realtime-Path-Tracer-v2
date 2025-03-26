#pragma once
#include "vec.hpp"
#include "util.hpp"

struct BRDF {
    virtual Vec eval(const Vec& n, const Vec& o, const Vec& i) const = 0;
    virtual void sample(const Vec& n, const Vec& o, Vec& i, double& pdf) const = 0;
    virtual bool isSpecular() const = 0;
};

// Ideal diffuse BRDF
struct DiffuseBRDF : public BRDF {
    DiffuseBRDF(Vec kd_) : kd(kd_) {}

    Vec eval(const Vec& n, const Vec& o, const Vec& i) const {
        return kd * (1.0 / PI);
    }

    /**
     * Sample using uniformRandomPSA
     */
    void sample(const Vec& n, const Vec& o, Vec& i, double& pdf) const {
        double z = sqrt(rng());
        double r = sqrt(1.0 - z * z);
        double phi = 2.0 * PI * rng();
        double x = r * cos(phi);
        double y = r * sin(phi);
        Vec u, v, w;
        createLocalCoord(n, u, v, w);
        i = u * x + v * y + w * z;
        pdf = clamp(i.dot(n)) / PI;
    }

    bool isSpecular() const { return false; }

    Vec kd;
};

// Ideal specular BRDF
struct SpecularBRDF : public BRDF {
    SpecularBRDF(Vec ks_) : ks(ks_) {}

    static Vec mirroredDirection(const Vec& n, const Vec& o) {
        return n * n.dot(o) * 2.0 - o;
    }

    Vec eval(const Vec& n, const Vec& o, const Vec& i) const {
        if (i == mirroredDirection(n, o)) {
            return ks * (1.0 / n.dot(i));
        }
        return Vec();
    }

    void sample(const Vec& n, const Vec& o, Vec& i, double& pdf) const {
        i = mirroredDirection(n, o);
        pdf = 1.0;
    }

    bool isSpecular() const { return true; }

    Vec ks;
};