#pragma once
#include "ray.hpp"
#include "brdf.hpp"

struct Shape {
	const BRDF& brdf;
	Vec e;

	Shape(const BRDF& brdf, Vec e) : brdf(brdf), e(e) {}

	virtual ~Shape() = default;

	virtual double intersect(const Ray& ray, Vec* point, Vec* normal) const = 0;

	virtual void sample(Vec& point, Vec& normal, double& pdf) const = 0;
};