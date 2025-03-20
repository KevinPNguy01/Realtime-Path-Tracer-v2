#pragma warning(disable : 4996) //_CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <limits>
#include <OpenImageDenoise/oidn.hpp>
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
    Sphere(5,  Vec(50,70.0,81.6),      Vec(50,50,50), blackSurf)   // Light
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

Vec receivedRadiance(const Ray& r, int depth);

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
    double pdf1;
    light.sample(y1, pdf1);

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
    pdf1 *= r_sq / ny.dot(w1_neg);
    Vec dirRadiance = light.e.mult(obj.brdf.eval(n, w1, o)) * visibility * clamp(n.dot(w1));

    // Russian roulette
    double p = depth <= maxDepth ? 1 : rrRate;

    if (samps > 1 && rng() < p) {
        // Sample new direction
        Vec w2;
        double pdf2;
        obj.brdf.sample(n, o, w2, pdf2);

        // Add radiance from new sampled direction
        Ray y2(x, w2);
        Vec refRadiance = reflectedRadiance(y2, depth + 1).mult(obj.brdf.eval(n, w2, o)) * clamp(n.dot(w2));
        return dirRadiance * (1.0 / (pdf1)) + refRadiance * (1.0 / (pdf2 * p));
    }

    return dirRadiance * (1.0 / (pdf1));
}

/*
 * KEY FUNCTION: radiance estimator
 */

Vec receivedRadiance(const Ray& r, int depth) {
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
        double p = depth <= maxDepth ? 1 : rrRate;
        if (samps > 1 && rng() < p) {
            // Sample new direction
            Vec i;
            double pdf;
            obj.brdf.sample(n, o, i, pdf);
            Ray Y(x, i);

            // Add radiance from new sampled direction
            rad = rad + receivedRadiance(Y, depth).mult(obj.brdf.eval(n, o, i)) * (clamp(n.dot(i)) / (pdf * p));
        }
        return rad;
    }

    // Otherwise, use our next event estimation
    return obj.e + reflectedRadiance(r, depth);
}


int main(int argc, char* argv[]) {
    unsigned int numThreads = std::thread::hardware_concurrency();
    rng.init(numThreads);

    int width = 480, height = 360;
    samps.store(argc == 2 ? atoi(argv[1]) / 4 : 1);
    Camera cam(50, 52, 295.6);
    Vec cx = cam.u, cy = cam.v;
    std::vector<Vec> c(width * height);

    std::atomic<int> workersDone = 0;
    auto pathTrace = [&height, &width, &c, &cam, &workersDone](int startY, int endY)-> void {
        int rowsFinished = 0;
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
                            Vec d = cam.u * (((sx + .5 + dx) / 2 + x) / width - .5) + cam.v * (((sy + .5 + dy) / 2 + y) / height - .5) + cam.w;
                            r = r + receivedRadiance(Ray(cam.pos, d.normalize()), 1) * (1. / samps);
                        }
                        c[i] = c[i] + Vec(clamp(r.x), clamp(r.y), clamp(r.z)) * (samps == 1 ? 1 : 0.25);
                    }
                }
            }
        }
        ++workersDone;
    };

    std::vector<std::thread> workers;
    int rowsPerWorker = (int) ((float)height / numThreads + 0.5);

    Window window(height, width);
    void* bits = window.bits;

    std::thread inputThread(handleInput, window.hwnd);
    centerMouse(window.hwnd);

    constexpr int FPS = 60;
    constexpr std::chrono::milliseconds frameDuration(1000 / FPS);
    auto previous = std::chrono::high_resolution_clock::now();

    oidn::DeviceRef device = oidn::newDevice();
    device.commit();

    while (1) { 
        auto start = std::chrono::high_resolution_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(start - previous);  

        c = std::vector<Vec>(height * width);

        workers.clear();
        workersDone = 0;
        for (int i = 0; i < numThreads; ++i) {
            workers.emplace_back(std::thread(pathTrace, i*rowsPerWorker, min(height, (i+1)*rowsPerWorker)));
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
            printf("Rendered with %d samples per pixel\n", samps.load());
            
            if (samps > 1) {
                // Create OIDN device
                oidn::DeviceRef device = oidn::newDevice();
                device.commit();

                // Allocate buffers using OIDN
                oidn::BufferRef colorBuffer = device.newBuffer(width * height * 3 * sizeof(float));
                oidn::BufferRef outputBuffer = device.newBuffer(width * height * 3 * sizeof(float));

                // Convert Vec (double) to OIDN-compatible float buffer
                float* colorImage = static_cast<float*>(colorBuffer.getData());
                for (int i = 0; i < width * height; i++) {
                    colorImage[i * 3 + 0] = static_cast<float>(c[i].x);
                    colorImage[i * 3 + 1] = static_cast<float>(c[i].y);
                    colorImage[i * 3 + 2] = static_cast<float>(c[i].z);
                }

                // Create OIDN filter
                oidn::FilterRef filter = device.newFilter("RT");  // Ray tracing denoiser
                filter.setImage("color", colorBuffer, oidn::Format::Float3, width, height);
                filter.setImage("output", outputBuffer, oidn::Format::Float3, width, height);
                filter.commit();

                // Execute denoising
                filter.execute();

                // Copy output back to original vector
                float* outputImage = static_cast<float*>(outputBuffer.getData());
                for (int i = 0; i < width * height; i++) {
                    c[i].x = static_cast<double>(outputImage[i * 3 + 0]);
                    c[i].y = static_cast<double>(outputImage[i * 3 + 1]);
                    c[i].z = static_cast<double>(outputImage[i * 3 + 2]);
                }
            }

            for (int i = 0; i < width * height; i++) {
                ((DWORD*)bits)[i] = RGB(toInt(c[i].x), toInt(c[i].y), toInt(c[i].z));
            }

            samps = min(128, samps * 2);
        }
        newInput.store(false);

        
        window.refresh();
        previous = start;
        std::this_thread::sleep_for(max(std::chrono::milliseconds(0), frameDuration - elapsedTime));
    }

    return 0;
}
