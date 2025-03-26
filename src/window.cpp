#pragma once
#include "window.hpp"
#include <stdio.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    Window* window = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (uMsg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_SIZE:
        window->refresh();
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void ShowConsoleCursor(bool showFlag)
{
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_CURSOR_INFO     cursorInfo;

    GetConsoleCursorInfo(out, &cursorInfo);
    cursorInfo.bVisible = showFlag; // set the cursor visibility
    SetConsoleCursorInfo(out, &cursorInfo);
}

Window::Window(int height, int width) : width(width), height(height) {
    const wchar_t CLASS_NAME[] = L"Realtime Raytracer";
    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    RECT rect = { 0, 0, width, height };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Realtime Raytracer",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, wc.hInstance, NULL
    );
    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    ShowWindow(hwnd, SW_SHOW);

    HDC hdcDest = GetDC(hwnd);
    ShowConsoleCursor(false);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HBITMAP hBitmap = CreateDIBSection(hdcDest, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    hdcMem = CreateCompatibleDC(hdcDest);
    SelectObject(hdcMem, hBitmap);
}

void Window::refresh() {
    BitBlt(GetDC(hwnd), 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);
}

void Window::proccessMessages() {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
        }
        else {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}