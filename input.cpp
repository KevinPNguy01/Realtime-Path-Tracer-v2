#pragma once
#include "input.hpp"
#include <chrono>
#include <thread>

std::atomic<POINT> lastMousePos;
boolean escapeKeyState = false;
boolean inFocus = true;
std::atomic<bool> input_flags[6];   // Flags indicating a key was held down during a frame.
std::atomic<int> dx, dy;
std::atomic<bool> newInput;

void centerMouse(HWND hwnd) {
    RECT rect;
    if (GetWindowRect(hwnd, &rect)) {
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        POINT center = { width / 2, height / 2 };
        ClientToScreen(hwnd, &center);
        SetCursorPos(center.x, center.y);
        lastMousePos.store(center);
    }  
}

void handleInput(HWND hwnd) {
    constexpr char keys[] = { 'W', 'A', 'S', 'D', VK_SPACE, VK_SHIFT };
    while (1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // Unfocus window by pressing escape.
        if ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0) {
            if (!escapeKeyState) {
                escapeKeyState = true;
                inFocus = !inFocus;
                centerMouse(hwnd);
            }
        }
        else {
            escapeKeyState = false;
        }
        if (!inFocus) continue;

        handleMouseInput(hwnd);
        // Raise flags for each of the keys that were held down during a frame.
        for (int i = 0; i < 6; ++i) {
            if (GetAsyncKeyState(keys[i]) & 0x8000) {
                input_flags[i].store(true);
                newInput.store(true);
            }
        }
    }
}

void handleMouseInput(HWND hwnd) {
    POINT currentMousePos;
    GetCursorPos(&currentMousePos);

    // Calculate the difference in cursor position
    dx.store(currentMousePos.x - lastMousePos.load().x);
    dy.store(currentMousePos.y - lastMousePos.load().y);

    // Store the current position for the next frame
    lastMousePos.store(currentMousePos);

    if (inFocus && (dx.load() != 0 || dy.load() != 0)) {
        newInput.store(true);
    }
}