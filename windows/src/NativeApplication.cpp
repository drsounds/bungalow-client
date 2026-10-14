#include "NativeApplication.h"

#include "MainElement.h"
#include "SPClassicSkin.h"
#include "SPMouseEventArgs.h"
#include "SPWindowElement.h"
#include "Space.h"
#include "Win32GraphicsContext.h"

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

        mainElement = new MainElement(windowElement);
        mainElement->setId("main");
        windowElement->appendChild(mainElement);
        resize();
        return true;
    }

    void NativeApplication::resize()
    {
        if (!windowElement || !mainElement)
            return;
        RECT client{};
        GetClientRect(windowHandle, &client);
        windowElement->setAbsoluteBounds({0, 0, client.right, client.bottom});
        windowElement->pack();
        mainElement->setY(HeaderHeight);
        mainElement->setHeight((std::max)(0, client.bottom - HeaderHeight));
        mainElement->pack();
    }

    void NativeApplication::paint(HDC dc)
    {
        if (!windowElement)
            return;
        graphics->setDeviceContext(dc);
        RECT client{};
        GetClientRect(windowHandle, &client);
        windowElement->draw(graphics, client.right, client.bottom);
    }

    void NativeApplication::mouseEvent(const char *name, int x, int y, int button)
    {
        if (!space || !windowElement)
            return;
        auto *args = new SPMouseEventArgs(button, x, y);
        space->dispatchEvent(name, args, windowElement);
        if (std::string(name) == "mouseup" && button == 0 && leftButtonDown)
        {
            auto *clickArgs = new SPMouseEventArgs(button, x, y);
            space->dispatchEvent("click", clickArgs, windowElement);
            leftButtonDown = false;
        }
        else if (std::string(name) == "mousedown" && button == 0)
        {
            leftButtonDown = true;
        }
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
}
