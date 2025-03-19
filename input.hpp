#pragma once
#include "window.hpp"
#include <atomic>
#include <Windows.h>

extern std::atomic<POINT>;
extern boolean escapeKeyState;
extern boolean inFocus;
extern std::atomic<bool> input_flags[6];   // Flags indicating a key was held down during a frame.
extern std::atomic<int> dx, dy;
extern std::atomic<bool> newInput;



void handleInput(HWND hwnd);

void handleMouseInput(HWND hwnd);

void centerMouse(HWND hwnd);