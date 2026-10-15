#pragma once

#include <Windows.h>

#include <memory>

class SpacifyApp;

namespace space
{
    class SPElement;
    class SPTextElement;
    class SPWindowElement;
    class Win32GraphicsContext;
    class Space;

    class NativeApplication
    {
    public:
        static constexpr int HeaderHeight = 40;

        NativeApplication();
        ~NativeApplication();

        bool initialize(HWND window);
        void resize();
        void paint(HDC dc);
        void mouseEvent(const char *name, int x, int y, int button = 0, bool commandDown = false);
        void keyEvent(int keyCode, bool commandDown);
        void command(WPARAM wParam, LPARAM lParam);
        void timer();

    private:
        HWND windowHandle = nullptr;
        Space *space = nullptr;
        SPWindowElement *windowElement = nullptr;
        SPElement *mainElement = nullptr;
        Win32GraphicsContext *graphics = nullptr;
        std::unique_ptr<SpacifyApp> app;
        bool leftButtonDown = false;
        bool rightButtonDown = false;
    };
}
