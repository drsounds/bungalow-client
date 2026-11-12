#include "NativeApplication.h"

#include "SPClassicSkin.h"
#include "SPElement.h"
#include "SPEventArgs.h"
#include "SPMouseEventArgs.h"
#include "SPTextElement.h"
#include "SPWindowElement.h"
#include "Space.h"
#include "Win32DragDrop.h"
#include "Win32GraphicsContext.h"
#ifdef BUNGALOW_WTL
#include "BungalowApp.h"
#include "ui/MainWindow.h"
#else
class BungalowApp {};
#endif

#include <commctrl.h>
#include <algorithm>
#include <string>

namespace space
{
    bool NativeApplication::initialize(HWND window)
    {
        INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_STANDARD_CLASSES};
        InitCommonControlsEx(&controls);
        windowHandle = window;
        space = new Space();
        graphics = new Win32GraphicsContext(window);
        space->setGraphicsContext(graphics);

        SPSkin *skin = new SPClassicSkin();
        windowElement = new SPWindowElement(space, skin);
        windowElement->setColorScheme(SP_COLOR_SCHEME_DARK);
        space->addWindow(windowElement);

#ifdef BUNGALOW_WTL
        app = std::make_unique<BungalowApp>();
        try {
            mainElement = app->initialize(space, windowElement);
        } catch (const std::exception &e) {
            MessageBoxA(window, e.what(), "Bungalow startup error", MB_OK | MB_ICONERROR);
            return false;
        }
#else
        mainElement = new SPTextElement(windowElement);
        mainElement->setId("main");
        static_cast<SPTextElement *>(mainElement)->setInnerText("Bungalow");
        mainElement->setHeight(32);
        windowElement->appendChild(mainElement);
#endif
        win32::registerDropTarget(window);
        resize();
        return true;
    }

    NativeApplication::NativeApplication() = default;

    NativeApplication::~NativeApplication()
    {
        if (windowHandle)
            win32::revokeDropTarget(windowHandle);
    }

    void NativeApplication::resize()
    {
        if (!windowElement || !mainElement)
            return;
        RECT client{};
        GetClientRect(windowHandle, &client);
        windowElement->setAbsoluteBounds({0, 0, client.right, client.bottom});
        mainElement->setX(0);
        mainElement->setWidth((std::max)(0, static_cast<int>(client.right)));
        mainElement->setY(0);
        mainElement->setHeight((std::max)(0, static_cast<int>(client.bottom)));
        mainElement->pack();
    }

    void NativeApplication::paint(HDC dc)
    {
        if (!windowElement)
            return;
        graphics->setDeviceContext(dc);
        graphics->beginPaint();
        RECT client{};
        GetClientRect(windowHandle, &client);
        windowElement->draw(graphics, client.right, client.bottom);
        graphics->endPaint();
        graphics->setDeviceContext(nullptr);
    }

    void NativeApplication::mouseEvent(const char *name, int x, int y, int button, bool commandDown)
    {
        if (!space || !windowElement)
            return;
        if (std::string(name) == "mousemove")
            space::SPElement::setCursorHint(space::SPElement::SPCursorHint::Arrow);
        auto *args = new SPMouseEventArgs(button, x, y, commandDown);
        space->dispatchEvent(name, args, windowElement);
        if (std::string(name) == "mouseup" &&
            ((button == 0 && leftButtonDown) || (button == 1 && rightButtonDown)))
        {
            auto *clickArgs = new SPMouseEventArgs(button, x, y, commandDown);
            space->dispatchEvent("click", clickArgs, windowElement);
            if (button == 0)
                leftButtonDown = false;
            else
                rightButtonDown = false;
        }
        else if (std::string(name) == "mousedown" && button == 0)
        {
            leftButtonDown = true;
        }
        else if (std::string(name) == "mousedown" && button == 1)
        {
            rightButtonDown = true;
        }
    }

    void NativeApplication::wheelEvent(int x, int y, int wheelDelta)
    {
        if (!space || !windowElement)
            return;
        UINT lines = 3;
        SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &lines, 0);
        if (lines == WHEEL_PAGESCROLL)
            lines = 20;
        // Precision touchpads send fractions of WHEEL_DELTA, which scroll smoothly.
        constexpr int lineHeight = 16;
        auto *args = new SPMouseEventArgs(0, x, y);
        args->setWheelDeltaY(-wheelDelta * static_cast<int>(lines) * lineHeight / WHEEL_DELTA);
        space->dispatchEvent("mousewheel", args, windowElement);
    }

    void NativeApplication::keyEvent(int keyCode, bool commandDown)
    {
        if (!space || !windowElement)
            return;
        if (keyCode == VK_UP)
            keyCode = SPKeyEventArgs::KeyUp;
        else if (keyCode == VK_DOWN)
            keyCode = SPKeyEventArgs::KeyDown;
        auto *args = new SPKeyEventArgs(keyCode, commandDown);
        const SPEvent event("keydown", args, nullptr);
        space->dispatchEvent("keydown", args, windowElement);
        space->processEvent(event);
    }

    void NativeApplication::timer()
    {
        if (graphics)
            graphics->thaw();
    }

    void NativeApplication::command(WPARAM wParam, LPARAM lParam)
    {
        if (graphics)
            graphics->handleCommand(wParam, lParam);
    }

    void NativeApplication::showWebInspector()
    {
#ifdef BUNGALOW_WTL
        if (app)
            app->showWebInspector();
#endif
    }

    void NativeApplication::reloadWebPage()
    {
#ifdef BUNGALOW_WTL
        if (app)
            app->reloadWebPage();
#endif
    }
}
