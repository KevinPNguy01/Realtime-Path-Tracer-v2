#pragma once
#include <OpenImageDenoise/oidn.hpp>
#include <windows.h>
#include "sphere.hpp"
#include "camera.hpp"
#include "scene.hpp"

class OIDNDenoiser {
public:
	int width, height;
	float *colorData, *albedoData, *normalData;

	OIDNDenoiser(int w, int h);

	void computeAuxiliary(const Sphere spheres[], const Camera& cam);
	void execute();

	void writeBits(void* bits);
private:
	oidn::DeviceRef device;
	oidn::BufferRef colorBuffer;
	oidn::BufferRef albedoBuffer;
	oidn::BufferRef normalBuffer;
};