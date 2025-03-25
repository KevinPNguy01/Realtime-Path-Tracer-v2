#pragma warning(disable : 4996) //_CRT_SECURE_NO_WARNINGS
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <limits>
#include "vec.hpp"
#include "brdf.hpp"
#include "sphere.hpp"
#include "window.hpp"
#include "input.hpp"
#include "camera.hpp"
#include "scene.hpp"
#include "denoiser.hpp"
#include "pathtracer.hpp"

constexpr int width = 640, height = 360;
constexpr int FPS = 60;
constexpr std::chrono::milliseconds frameDuration(1000 / FPS);
std::atomic<int> samps(1);

int main() {
    rng.init(std::thread::hardware_concurrency());
    auto previous = std::chrono::high_resolution_clock::now();

    // Wrapper classes essential for rendering
    Camera cam(0, 5, 13);
    Window window(height, width);
    OIDNDenoiser denoiser(width, height);
    PathTracer pathTracer(denoiser.colorData, width, height, cam, window);

    // Separate thread for handling mouse and keyboard inputs
    std::thread inputThread(handleInput, window.hwnd);

    while (1) { 
        auto start = std::chrono::high_resolution_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(start - previous);  

        // Path trace for samps samples
        pathTracer.pathTrace(samps);

        // Skip rendering and reset samples
        if (newInput.load() && inFocus && samps > 1) {
            samps = 1;
        }
        else {
            // Generate auxiliary buffers on second frame
            if (samps == 2) {
                denoiser.computeAuxiliary(shapes, cam);
            }
            // Denoise on second frame and beyond
            if (samps > 1) {
                denoiser.execute();
            }

            printf("Rendered with %d samples per pixel\n", samps.load() == 1 ? 1 : (samps.load() / 2) * 4);
            denoiser.writeBits(window.bits);
            window.refresh();

            samps = min(128, samps * 2);
        }

        // Update camera based on user input and reset mouse position
        updateCamera(cam);
        centerMouse(window.hwnd);

        previous = start;
        std::this_thread::sleep_for(max(std::chrono::milliseconds(0), frameDuration - elapsedTime));
    }

    return 0;
}
