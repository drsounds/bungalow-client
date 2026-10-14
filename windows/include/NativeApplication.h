#pragma once

#include <Windows.h>

namespace space
{
    class SPWindowElement;
    class Win32GraphicsContext;
    class Space;

    class NativeApplication
    {
    public:
        bool initialize(HWND window);
        void resize();
        void paint(HDC dc);
        void mouseEvent(const char *name, int x, int y, int button = 0);
        void command(WPARAM wParam, LPARAM lParam);
        void timer();

    private:
        HWND windowHandle = nullptr;
        Space *space = nullptr;
        SPWindowElement *windowElement = nullptr;
        Win32GraphicsContext *graphics = nullptr;
        bool leftButtonDown = false;
    };
}
