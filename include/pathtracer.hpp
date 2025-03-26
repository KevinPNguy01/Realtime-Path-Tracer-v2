#pragma once
#include "camera.hpp"
#include "ray.hpp"
#include "util.hpp"
#include "scene.hpp"
#include "sphere.hpp"
#include "input.hpp"
#include "window.hpp"
#include <thread>

extern std::atomic<int> workersDone;

class PathTracer {
public:
	PathTracer(float* data, int width, int height, Camera& camera, Window& window);
	void pathTrace(int samps);

private:
	Window& window;
	Camera& camera;
	float* data;
	int width, height, numThreads;
};

void pathTraceThread(float* data, int width, int height, int samps, int startY, int endY, const Camera& camera);

Vec reflectedRadiance(const Ray& r, int depth, bool firstFrame);
Vec receivedRadiance(const Ray& r, int depth, bool firstFrame);