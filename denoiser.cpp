#pragma once
#include "denoiser.hpp"

OIDNDenoiser::OIDNDenoiser(int w, int h) : width(w), height(h), device(oidn::newDevice()) {
	device.commit();
	colorBuffer = device.newBuffer(width * height * 3 * sizeof(float));
	albedoBuffer = device.newBuffer(width * height * 3 * sizeof(float));
	normalBuffer = device.newBuffer(width * height * 3 * sizeof(float));

	colorData = static_cast<float*>(colorBuffer.getData());
	albedoData = static_cast<float*>(albedoBuffer.getData());
	normalData = static_cast<float*>(normalBuffer.getData());
}

void OIDNDenoiser::computeAuxiliary(const Sphere spheres[], const Camera& cam) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const int i = (height - y - 1) * width + x;
            Vec d = cam.u * ((x + .5) / width - .5) + cam.v * ((y + .5) / height - .5) + cam.w;
            Ray ray(cam.pos, d.normalize());

            int id;
            double t;
            if (intersect(ray, t, id)) {
                Vec x = ray.o + ray.d * t;
                Vec normal = (x - spheres[id].p).normalize();
                normalData[i * 3 + 0] = static_cast<float>(normal.x);
                normalData[i * 3 + 1] = static_cast<float>(normal.y);
                normalData[i * 3 + 2] = static_cast<float>(normal.z);

                while (spheres[id].brdf.isSpecular()) {
                    ray = Ray(x, dynamic_cast<const SpecularBRDF*>(&spheres[id].brdf)->mirroredDirection(normal, ray.d * -1));
                    if (!intersect(ray, t, id)) break;
                    x = ray.o + ray.d * t;
                    normal = (x - spheres[id].p).normalize();
                }
                if (!spheres[id].brdf.isSpecular()) {
                    Vec kd = dynamic_cast<const DiffuseBRDF*>(&spheres[id].brdf)->kd;
                    albedoData[i * 3 + 0] = static_cast<float>(kd.x);
                    albedoData[i * 3 + 1] = static_cast<float>(kd.y);
                    albedoData[i * 3 + 2] = static_cast<float>(kd.z);
                }
            }
        }
    }
}

void OIDNDenoiser::execute() {
    oidn::FilterRef filter = device.newFilter("RT");
    filter.setImage("color", colorBuffer, oidn::Format::Float3, width, height);
    filter.setImage("normal", normalBuffer, oidn::Format::Float3, width, height);
    filter.setImage("albedo", albedoBuffer, oidn::Format::Float3, width, height);
    filter.setImage("output", colorBuffer, oidn::Format::Float3, width, height);
    filter.commit();
    filter.execute();

    const char* errorMessage;
    oidn::Error error = device.getError(errorMessage);
    if (error != oidn::Error::None) {
        printf("OIDN Error: %s\n", errorMessage);
    }
}

void OIDNDenoiser::writeBits(void* bits) {
    for (int i = 0; i < width * height; i++) {
        ((DWORD*)bits)[i] = RGB(toInt(colorData[i * 3 + 0]), toInt(colorData[i * 3 + 1]), toInt(colorData[i * 3 + 2]));
    }
}