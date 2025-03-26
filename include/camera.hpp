#pragma once
#include "vec.hpp"

class Camera {

	float yaw, pitch;

	void calculateBasis();

public:
	static enum direction { FORWARD, LEFT, BACKWARD, RIGHT, UP, DOWN };

	Vec u, v, w;
	Vec pos;

	Camera(double x, double y, double z);

	void move(direction dir, float amount);
	void rotateYaw(float angle);
	void rotatePitch(float angle);
};