#pragma warning(disable : 4996) //_CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include "vec.hpp"
#include "brdf.hpp"
#include "sphere.hpp"
#include "window.hpp"
#include "input.hpp"
#include "camera.hpp"

/*
 * Scene configuration
 */

 // Pre-defined BRDFs
const DiffuseBRDF leftWall(Vec(.75, .25, .25)),
rightWall(Vec(.25, .25, .75)),
otherWall(Vec(.75, .75, .75)),
blackSurf(Vec(0.0, 0.0, 0.0)),
brightSurf(Vec(0.9, 0.9, 0.9));
const SpecularBRDF shinySurf(Vec(0.999, 0.999, 0.999));

// Scene: list of spheres
const Sphere spheres[] = {
    Sphere(1e5,  Vec(1e5 + 1,40.8,81.6),   Vec(),         leftWall),   // Left
    Sphere(1e5,  Vec(-1e5 + 99,40.8,81.6), Vec(),         rightWall),  // Right
    Sphere(1e5,  Vec(50,40.8, 1e5),      Vec(),         otherWall),  // Back
    Sphere(1e5,  Vec(50, 1e5, 81.6),     Vec(),         otherWall),  // Bottom
    Sphere(1e5,  Vec(50,-1e5 + 81.6,81.6), Vec(),         otherWall),  // Top
    Sphere(16.5, Vec(27,16.5,47),        Vec(),         brightSurf), // Ball 1
    Sphere(16.5, Vec(73,16.5,78),        Vec(),         shinySurf), // Ball 2
    Sphere(5.0,  Vec(50,70.0,81.6),      Vec(50,50,50), blackSurf)   // Light
};

/*
 * Global functions
 */

bool intersect(const Ray& r, double& t, int& id) {
    double n = sizeof(spheres) / sizeof(Sphere), d, inf = t = 1e20;
    for (int i = int(n); i--;) if ((d = spheres[i].intersect(r)) && d < t) { t = d; id = i; }
    return t < inf;
}

std::atomic<int> samps;
constexpr int maxDepth = 2;
constexpr double rrRate = 0;

Vec reflectedRadiance(const Ray& r, int depth) {
    double t;                                   // Distance to intersection
    int id = 0;                                 // id of intersected sphere

    if (!intersect(r, t, id)) return Vec();   // if miss, return black
    const Sphere& obj = spheres[id];            // the hit object

    Vec x = r.o + r.d * t;                        // The intersection point
    Vec o = (Vec() - r.d).normalize();          // The outgoing direction (= -r.d)

    Vec n = (x - obj.p).normalize();            // The normal direction
    if (n.dot(o) < 0) n = n * -1.0;

    /*
    Tips

    1. Other useful quantities/variables:
    Vec Le = obj.e;                             // Emitted radiance
    const BRDF &brdf = obj.brdf;                // Surface BRDF at x

    2. Call brdf.sample() to sample an incoming direction and continue the recursion
    */

    // Direct radiance
    const Sphere& light = spheres[7];

    // Sample random point on the light source
    Vec y1;
    double pdf;
    light.sample(y1, pdf);

    // Some calculations we need for radiance
    Vec xToY = (y1 - x);
    Vec w1 = (Vec(xToY)).normalize();
    Vec w1_neg = Vec(-w1.x, -w1.y, -w1.z);
    double r_sq = (xToY).dot(xToY);
    Vec ny = (y1 - light.p).normalize();

    // Mutually visible if rays from each object intersect each other
    int id2;
    int visibility = intersect(Ray(x, w1), t, id2) && id2 == 7 && intersect(Ray(y1, w1_neg), t, id2) && id2 == id ? 1 : 0;

    // Final calculation for direct radiance
    Vec reflRad = light.e.mult(obj.brdf.eval(n, w1, o)) * visibility * clamp(n.dot(w1)) * clamp(ny.dot(w1_neg)) * (1.0 / (r_sq * pdf));

    // Russian roulette
    double p = depth <= (samps == 1 ? 1 : maxDepth) ? 1 : rrRate;
    if (rng() < p) {
        // Sample new direction
        Vec w2;
        double pdf2;
        obj.brdf.sample(n, o, w2, pdf2);

        // Add radiance from new sampled direction
        Ray y2(x, w2);
        reflRad = reflRad + reflectedRadiance(y2, depth + 1).mult(obj.brdf.eval(n, w2, o)) * clamp(n.dot(w2)) * (1.0 / (pdf2 * p));
    }

    return reflRad;
}

/*
 * KEY FUNCTION: radiance estimator
 */

Vec receivedRadiance(const Ray& r, int depth, bool flag) {
    double t;                                   // Distance to intersection
    int id = 0;                                 // id of intersected sphere

    if (!intersect(r, t, id)) return Vec();   // if miss, return black
    const Sphere& obj = spheres[id];            // the hit object

    Vec x = r.o + r.d * t;                        // The intersection point
    Vec o = (Vec() - r.d).normalize();          // The outgoing direction (= -r.d)

    Vec n = (x - obj.p).normalize();            // The normal direction
    if (n.dot(o) < 0) n = n * -1.0;

    // If specular, use the radiance calculation from task 2
    if (obj.brdf.isSpecular()) {
        // Emitted radiance
        Vec rad = obj.e;

        // Russian roulette
        double p = depth <= (samps == 1 ? 1 : maxDepth) ? 1 : rrRate;
        if (rng() < p) {
            // Sample new direction
            Vec i;
            double pdf;
            obj.brdf.sample(n, o, i, pdf);
            Ray Y(x, i);

            // Add radiance from new sampled direction
            rad = rad + receivedRadiance(Y, depth + 1, false).mult(obj.brdf.eval(n, o, i)) * (clamp(n.dot(i)) / (pdf * p));
        }
        return rad;
    }

    // Otherwise, use our next event estimation
    return obj.e + reflectedRadiance(r, depth);
}


int main(int argc, char* argv[]) {
    unsigned int numThreads = std::thread::hardware_concurrency();
    rng.init(numThreads);

    int w = 480, h = 360;
    samps.store(argc == 2 ? atoi(argv[1]) / 4 : 1);
    Camera cam(50, 52, 295.6);
    Vec cx = cam.u, cy = cam.v;
    std::vector<Vec> c(w * h);

    std::atomic<int> workersDone = 0;
    auto pathTrace = [&h, &w, &c, &cam, &workersDone](int startY, int endY)-> void {
        int rowsFinished = 0;
        for (int y = startY; y < endY; y++) {
            for (int x = 0; x < w; x++) {
                const int i = (h - y - 1) * w + x;
                for (int sy = 0; sy < (samps == 1 ? 1 : 2); ++sy) {
                    for (int sx = 0; sx < (samps == 1 ? 1 : 2); ++sx) {
                        if (newInput.load() && samps > 1) {
                            ++workersDone;
                            return;
                        }

                        Vec r;
                        for (int s = 0; s < samps; s++) {
                            double r1 = 2 * rng(), dx = r1 < 1 ? sqrt(r1) - 1 : 1 - sqrt(2 - r1);
                            double r2 = 2 * rng(), dy = r2 < 1 ? sqrt(r2) - 1 : 1 - sqrt(2 - r2);
                            Vec d = cam.u * (((sx + .5 + dx) / 2 + x) / w - .5) + cam.v * (((sy + .5 + dy) / 2 + y) / h - .5) + cam.w;
                            r = r + receivedRadiance(Ray(cam.pos, d.normalize()), 1, true) * (1. / samps);
                        }
                        c[i] = c[i] + Vec(clamp(r.x), clamp(r.y), clamp(r.z)) * (samps == 1 ? 1 : 0.25);
                    }
                }
            }
        }
        ++workersDone;
    };

    std::vector<std::thread> workers;
    int rowsPerWorker = (int) ((float)h / numThreads + 0.5);

    Window window(h, w);
    void* bits = window.bits;

    std::thread inputThread(handleInput, window.hwnd);
    centerMouse(window.hwnd);

    constexpr int FPS = 60;
    constexpr std::chrono::milliseconds frameDuration(1000 / FPS);
    auto previous = std::chrono::high_resolution_clock::now();

    while (1) { 
        auto start = std::chrono::high_resolution_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(start - previous);  

        c = std::vector<Vec>(h * w);

        workers.clear();
        workersDone = 0;
        for (int i = 0; i < numThreads; ++i) {
            workers.emplace_back(std::thread(pathTrace, i*rowsPerWorker, min(h, (i+1)*rowsPerWorker)));
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

        for (int i = 0; i < 6; ++i) {
            if (input_flags[i].load()) {
                cam.move(static_cast<Camera::direction>(i), 4);
                input_flags[i].store(false);
            }
        }

        if (newInput.load() && inFocus && samps > 1) {
            samps = 1;
            const float sensitivity = 0.2f;
            cam.rotatePitch(-sensitivity * dy);
            cam.rotateYaw(-sensitivity * dx);
            centerMouse(window.hwnd);
        }
        else {
            samps = min(128, samps * 2);
            for (int i = 0; i < w * h; i++) {
                ((DWORD*)bits)[i] = RGB(toInt(c[i].x), toInt(c[i].y), toInt(c[i].z));
            }
        }
        newInput.store(false);

        
        window.refresh();
        previous = start;
        std::this_thread::sleep_for(max(std::chrono::milliseconds(0), frameDuration - elapsedTime));
    }

    return 0;
}
