#pragma once
#include <windows.h>

class Window {
public:
    void* bits;
    
    Window(int height, int width);

    void refresh();
    void proccessMessages();

private:
    HWND hwnd;
    HDC hdcMem;
	int width, height;
};