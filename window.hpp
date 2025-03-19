#pragma once
#include <windows.h>

class Window {
public:
    void* bits;
    HWND hwnd;
    
    Window(int height, int width);

    void refresh();
    void proccessMessages();

private:
    HDC hdcMem;
	int width, height;
};