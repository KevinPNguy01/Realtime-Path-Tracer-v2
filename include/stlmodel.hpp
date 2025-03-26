#pragma once
#include <vector>
#include <fstream>
#include <iostream>
#include <limits>
#include <cmath>
#include "sphere.hpp"
#include "triangle.hpp"

class STLModel : public Shape {
public:
    std::vector<Triangle> triangles;
    Vec pos;
    double maxDist, totalSurfaceArea;
    Sphere boundingSphere;

    STLModel(const std::string& filepath, const BRDF& brdf, Vec pos, Vec e = Vec(), bool normalize = true, double scale = 1);

private:
    std::vector<double> cdf;

    void loadSTL(const std::string& filepath);

    void normalizeModel();

    double intersect(const Ray& ray, Vec* point, Vec* normal) const override;

    void sample(Vec& point, Vec& normal, double& pdf) const override;

    void computeSurfaceAreas();
};