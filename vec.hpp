#pragma once
#include <cmath>

const double epsilon = pow(10, -12);

struct Vec {
    double x, y, z;

    Vec(double x_ = 0, double y_ = 0, double z_ = 0) { x = x_; y = y_; z = z_; }

    Vec operator+ (const Vec& b) const { return Vec(x + b.x, y + b.y, z + b.z); }
    Vec operator- (const Vec& b) const { return Vec(x - b.x, y - b.y, z - b.z); }
    Vec operator* (double b) const { return Vec(x * b, y * b, z * b); }
    bool operator== (const Vec& b) const { return abs(x - b.x) < epsilon && abs(y - b.y) < epsilon && abs(z - b.z) < epsilon; }

    Vec mult(const Vec& b) const { return Vec(x * b.x, y * b.y, z * b.z); }
    Vec& normalize() { return *this = *this * (1.0 / sqrt(x * x + y * y + z * z)); }
    double dot(const Vec& b) const { return x * b.x + y * b.y + z * b.z; }
    Vec cross(const Vec& b) const { return Vec(y * b.z - z * b.y, z * b.x - x * b.z, x * b.y - y * b.x); }
    double length() const { return sqrt(x * x + y * y + z * z); }
};