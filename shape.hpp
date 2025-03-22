#pragma once
#include "ray.hpp"
#include "brdf.hpp"

struct Shape {
	const BRDF& brdf;
	Vec e;

	Shape(const BRDF& brdf, Vec e) : brdf(brdf), e(e) {}

	virtual ~Shape() = default;

	virtual double intersect(const Ray& ray) const = 0;

	virtual void sample(Vec& point, double& pdf) const = 0;

	virtual Vec normal(const Vec& point) const = 0;
};