#pragma once
#include <windows.h>

class Window {
public:
    void* bits;
    HWND hwnd;
    int width, height;
    
    Window(int height, int width);

    void refresh();
    void proccessMessages();

private:
    HDC hdcMem;
};