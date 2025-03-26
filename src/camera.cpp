#pragma once
#include "camera.hpp"

# define PI 3.14159265358979323846

double degreesToRadians(double degrees) {
    return degrees * (PI / 180.0);
}

Camera::Camera(double x, double y, double z) : pos(x, y, z) {
    yaw = -90;
    pitch = 0;
    calculateBasis();
}

void Camera::calculateBasis() {
    Vec forward(
        cos(degreesToRadians(yaw)) * cos(degreesToRadians(pitch)),
        sin(degreesToRadians(pitch)),
        sin(degreesToRadians(yaw)) * cos(degreesToRadians(pitch))
    );

    w = forward.normalize();
    u = Vec(0, 1, 0).cross(w);
    v = w.cross(u);
}

void Camera::move(direction dir, float amount) {
    Vec delta;
    switch (dir) {
    case UP:
        delta = Vec(0, 1, 0);
        break;
    case DOWN:
        delta = Vec(0, -1, 0);
        break;
    case LEFT:
        delta = u * -1;
        break;
    case RIGHT:
        delta = u;
        break;
    case FORWARD:
        delta = w.mult(Vec(1, 0, 1));
        break;
    case BACKWARD:
        delta = w.mult(Vec(-1, 0, -1));
        break;
    }

    pos = pos + delta * amount;
}

void Camera::rotateYaw(float angle) {
    yaw += angle;
    calculateBasis();
}

void Camera::rotatePitch(float angle) {
    pitch += angle;
    calculateBasis();
}