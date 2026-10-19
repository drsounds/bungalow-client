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
        // The caption (menu, title, window buttons) overlays the top of the app header,
        // which reserves this much top padding for it (see MainElement::layout).
        static constexpr int CaptionHeight = 25;

        NativeApplication();
        ~NativeApplication();

        bool initialize(HWND window);
        void resize();
        void paint(HDC dc);
        void mouseEvent(const char *name, int x, int y, int button = 0, bool commandDown = false);
        void keyEvent(int keyCode, bool commandDown);
        void command(WPARAM wParam, LPARAM lParam);
        void timer();
        // Develop menu: act on the visible web page or app.
        void showWebInspector();
        void reloadWebPage();

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
