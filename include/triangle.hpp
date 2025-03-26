#pragma once
#include "shape.hpp"
#include "vec.hpp"
#include "brdf.hpp"
#include "ray.hpp"

struct Triangle : public Shape {
	Vec v0, v1, v2;
	Vec n;

	Triangle(Vec v0, Vec v1, Vec v2, Vec e, const BRDF& brdf);

	double intersect(const Ray& r, Vec* point, Vec* normal) const override;

	void sample(Vec& point, Vec& normal, double& pdf) const override;

	double area() const;
};