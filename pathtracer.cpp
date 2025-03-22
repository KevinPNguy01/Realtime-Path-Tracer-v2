#pragma once
#include "pathtracer.hpp"

std::atomic<int> workersDone = 0;
constexpr int maxDepth = 2;
constexpr double rrRate = 0.5;

PathTracer::PathTracer(float* data, int width, int height, Camera& camera, Window& window)
    : data(data), width(width), height(height), camera(camera), window(window), numThreads(std::thread::hardware_concurrency()) {
}

void PathTracer::pathTrace(int samps) {
    memset(data, 0, width * height * 3 * sizeof(float));
    std::vector<std::thread> workers;
    int rowsPerWorker = (int)((float)height / numThreads + 0.5);
    workersDone.store(0);
    for (int i = 0; i < numThreads; ++i) {
        workers.emplace_back(std::thread(pathTraceThread, data, width, height, samps, i * rowsPerWorker, min(height, (i + 1) * rowsPerWorker), camera));
    }

    while (workersDone < numThreads) {
        window.proccessMessages();
        if (inFocus) {
            while (ShowCursor(FALSE) > 0);
        }
        else {
            while (ShowCursor(TRUE) < 0);
        }
    }

    for (auto& worker : workers) {
        worker.join();
    }
}

void pathTraceThread(float* data, int width, int height, int samps, int startY, int endY, const Camera& camera) {
    for (int y = startY; y < endY; y++) {
        for (int x = 0; x < width; x++) {
            const int i = (height - y - 1) * width + x;
            for (int sy = 0; sy < (samps == 1 ? 1 : 2); ++sy) {
                for (int sx = 0; sx < (samps == 1 ? 1 : 2); ++sx) {
                    Vec r;
                    for (int s = 0; s < samps; s++) {
                        if (newInput.load() && samps > 1) {
                            ++workersDone;
                            return;
                        }
                        double r1 = 2 * rng(), dx = r1 < 1 ? sqrt(r1) - 1 : 1 - sqrt(2 - r1);
                        double r2 = 2 * rng(), dy = r2 < 1 ? sqrt(r2) - 1 : 1 - sqrt(2 - r2);
                        Vec d = camera.u * (((sx + .5) / 2 + x) / width - .5) + camera.v * (((sy + .5) / 2 + y) / height - .5) + camera.w;
                        r = r + receivedRadiance(Ray(camera.pos, d.normalize()), 1, samps == 1) * (1. / samps);
                    }
                    Vec color = Vec(clamp(r.x), clamp(r.y), clamp(r.z)) * (samps == 1 ? 1 : 0.25);
                    data[i * 3 + 0] += static_cast<float>(color.x);
                    data[i * 3 + 1] += static_cast<float>(color.y);
                    data[i * 3 + 2] += static_cast<float>(color.z);
                }
            }
        }
    }
    ++workersDone;
}

Vec reflectedRadiance(const Ray& r, int depth, bool firstFrame) {
    double t;                                   // Distance to intersection
    int id = 0;                                 // id of intersected sphere

    if (!intersect(r, t, id)) return Vec();   // if miss, return black
    const Shape* obj = shapes[id];            // the hit object

    Vec x = r.o + r.d * t;                        // The intersection point
    Vec o = (Vec() - r.d).normalize();          // The outgoing direction (= -r.d)

    Vec n = obj->normal(x);           // The normal direction
    if (n.dot(o) < 0) n = n * -1.0;

    /*
    Tips

    1. Other useful quantities/variables:
    Vec Le = obj.e;                             // Emitted radiance
    const BRDF &brdf = obj.brdf;                // Surface BRDF at x

    2. Call brdf.sample() to sample an incoming direction and continue the recursion
    */

    // Direct radiance
    int lightId = 0;
    const Shape* light = shapes[lightId];

    // Sample random point on the light source
    Vec y1;
    double pdf1;
    light->sample(y1, pdf1);

    // Some calculations we need for radiance
    Vec xToY = (y1 - x);
    Vec w1 = (Vec(xToY)).normalize();
    Vec w1_neg = Vec(-w1.x, -w1.y, -w1.z);
    double r_sq = (xToY).dot(xToY);
    Vec ny = light->normal(y1);

    // Mutually visible if rays from each object intersect each other
    int id2;
    int visibility = intersect(Ray(x, w1), t, id2) && id2 == lightId && intersect(Ray(y1, w1_neg), t, id2) && id2 == id ? 1 : 0;

    // Final calculation for direct radiance
    pdf1 *= r_sq / ny.dot(w1_neg);
    Vec dirRadiance = light->e.mult(obj->brdf.eval(n, w1, o)) * visibility * clamp(n.dot(w1));

    // Russian roulette
    double p = depth <= maxDepth ? 1 : rrRate;

    if (!firstFrame && rng() < p) {
        // Sample new direction
        Vec w2;
        double pdf2;
        obj->brdf.sample(n, o, w2, pdf2);

        // Add radiance from new sampled direction
        Ray y2(x, w2);
        Vec refRadiance = reflectedRadiance(y2, depth + 1, firstFrame).mult(obj->brdf.eval(n, w2, o)) * clamp(n.dot(w2));
        return dirRadiance * (1.0 / (pdf1)) + refRadiance * (1.0 / (pdf2 * p));
    }

    return dirRadiance * (1.0 / (pdf1));
}

/*
 * KEY FUNCTION: radiance estimator
 */

Vec receivedRadiance(const Ray& r, int depth, bool firstFrame) {
    double t;                                   // Distance to intersection
    int id = 0;                                 // id of intersected sphere

    if (!intersect(r, t, id)) return Vec();   // if miss, return black
    const Shape* obj = shapes[id];            // the hit object

    Vec x = r.o + r.d * t;                        // The intersection point
    Vec o = (Vec() - r.d).normalize();          // The outgoing direction (= -r.d)

    Vec n = obj->normal(x);            // The normal direction
    if (n.dot(o) < 0) n = n * -1.0;

    // If specular, use the radiance calculation from task 2
    if (obj->brdf.isSpecular()) {
        // Emitted radiance
        Vec rad = obj->e;

        // Russian roulette
        double p = depth <= maxDepth ? 1 : rrRate;
        if (!firstFrame && rng() < p) {
            // Sample new direction
            Vec i;
            double pdf;
            obj->brdf.sample(n, o, i, pdf);
            Ray Y(x, i);

            // Add radiance from new sampled direction
            rad = rad + receivedRadiance(Y, depth, firstFrame).mult(obj->brdf.eval(n, o, i)) * (clamp(n.dot(i)) / (pdf * p));
        }
        return rad;
    }

    // Otherwise, use our next event estimation
    return obj->e + reflectedRadiance(r, depth, firstFrame);
}