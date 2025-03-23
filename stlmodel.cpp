#pragma once
#include "stlmodel.hpp"

STLModel::STLModel(const std::string& filepath, const BRDF& brdf, Vec pos, Vec e, bool normalize, double scale)
    : Shape(brdf, e), pos(pos), boundingSphere(1, pos, e, brdf) {
    loadSTL(filepath);
    if (normalize) {
        normalizeModel();
    }
    // Shift model into position
    for (auto& tri : triangles) {
        tri.v0 = (tri.v0 * scale + pos);
        tri.v1 = (tri.v1 * scale + pos);
        tri.v2 = (tri.v2 * scale + pos);
    }
    maxDist *= scale;
    boundingSphere.rad = maxDist;
    computeSurfaceAreas();
}

void STLModel::loadSTL(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file: " << filepath << "\n";
        return;
    }

    file.seekg(80); // Skip header
    uint32_t numTriangles;
    file.read(reinterpret_cast<char*>(&numTriangles), sizeof(numTriangles));

    Vec min(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec max(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());

    for (uint32_t i = 0; i < numTriangles; ++i) {
        float normal_f[3], v0_f[3], v1_f[3], v2_f[3]; // Temporary storage in float
        file.read(reinterpret_cast<char*>(normal_f), sizeof(normal_f));
        file.read(reinterpret_cast<char*>(v0_f), sizeof(v0_f));
        file.read(reinterpret_cast<char*>(v1_f), sizeof(v1_f));
        file.read(reinterpret_cast<char*>(v2_f), sizeof(v2_f));
        file.ignore(2); // Attribute byte count

        // Convert float to double
        Vec v0(v0_f[0], v0_f[2], v0_f[1]);
        Vec v1(v1_f[0], v1_f[2], v1_f[1]);
        Vec v2(v2_f[0], v2_f[2], v2_f[1]);
        triangles.emplace_back(v0, v1, v2, Vec(), brdf);
        for (const Vec& v : { v0, v1, v2 }) {
            min = Vec(std::min(min.x, v.x), std::min(min.y, v.y), std::min(min.z, v.z));
            max = Vec(std::max(max.x, v.x), std::max(max.y, v.y), std::max(max.z, v.z));
        }

        maxDist = std::max(std::max(max.x - min.x, max.y - min.y), max.z - min.z);
    }

    // Center model
    Vec center = (min + max) * 0.5f;
    for (auto& tri : triangles) {
        tri.v0 = (tri.v0 - center);
        tri.v1 = (tri.v1 - center);
        tri.v2 = (tri.v2 - center);
    }
}

void STLModel::normalizeModel() {
    // Scale the model to be unit size
    double scale = 1.0 / maxDist;
    for (auto& tri : triangles) {
        tri.v0 = tri.v0 * scale;
        tri.v1 = tri.v1 * scale;
        tri.v2 = tri.v2 * scale;
    }
    maxDist = 1;
}

double STLModel::intersect(const Ray& ray, Vec* point, Vec* normal) const {
    double inf = 1e20;
    if (!boundingSphere.intersect(ray, 0, 0)) return inf;

    double d, t = inf;
    Vec pos, norm;
    for (const auto& triangle : triangles) {
        if ((d = triangle.intersect(ray, &pos, &norm)) && d < t) {
            t = d;
            if (point && normal) {
                *point = pos, * normal = norm;
            }
        }
    }
    return t;
}

void STLModel::sample(Vec& point, Vec& normal, double& pdf) const {
    if (triangles.empty()) {
        return;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float r = dist(gen);

    size_t index = std::lower_bound(cdf.begin(), cdf.end(), r) - cdf.begin();
    const Triangle& tri = triangles[index];

    float u = dist(gen);
    float v = dist(gen);
    tri.sample(point, normal, pdf);
    pdf = 1.0 / totalSurfaceArea;
}

void STLModel::computeSurfaceAreas() {
    totalSurfaceArea = 0.0f;
    cdf.clear();

    for (const auto& tri : triangles) {
        totalSurfaceArea += tri.area();
        cdf.push_back(totalSurfaceArea);
    }

    for (double& value : cdf) {
        value /= totalSurfaceArea;
    }
}