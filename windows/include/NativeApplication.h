#pragma once

#include <Windows.h>

namespace space
{
    class SPTextElement;
    class SPWindowElement;
    class Win32GraphicsContext;
    class Space;

    class NativeApplication
    {
    public:
        static constexpr int HeaderHeight = 40;

        bool initialize(HWND window);
        void resize();
        void paint(HDC dc);
        void mouseEvent(const char *name, int x, int y, int button = 0);
        void keyEvent(int keyCode, bool commandDown);
        void command(WPARAM wParam, LPARAM lParam);
        void timer();

    private:
        HWND windowHandle = nullptr;
        Space *space = nullptr;
        SPWindowElement *windowElement = nullptr;
        SPTextElement *mainElement = nullptr;
        Win32GraphicsContext *graphics = nullptr;
        bool leftButtonDown = false;
        bool rightButtonDown = false;
    };
}
