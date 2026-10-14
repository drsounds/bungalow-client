#include <atlbase.h>
#include <atlapp.h>
#include <atlframe.h>
#include <atltypes.h>
#include <atlcrack.h>

namespace WTL
{
    using ::CPoint;
    using ::CSize;
}

#include "NativeApplication.h"

using namespace ATL;
CAppModule _Module;

namespace
{
    class MainFrame : public CFrameWindowImpl<MainFrame>
    {
    public:
        DECLARE_FRAME_WND_CLASS(_T("BungalowNativeWtlWindow"), 0)

        space::NativeApplication app;

        BEGIN_MSG_MAP(MainFrame)
            MSG_WM_CREATE(OnCreate)
            MSG_WM_SIZE(OnSize)
            MSG_WM_PAINT(OnPaint)
            MSG_WM_LBUTTONDOWN(OnLeftButtonDown)
            MSG_WM_LBUTTONUP(OnLeftButtonUp)
            MSG_WM_MOUSEMOVE(OnMouseMove)
            MSG_WM_TIMER(OnTimer)
            MESSAGE_HANDLER(WM_COMMAND, OnCommand)
            CHAIN_MSG_MAP(CFrameWindowImpl<MainFrame>)
        END_MSG_MAP()

        LRESULT OnCreate(LPCREATESTRUCT)
        {
            return app.initialize(m_hWnd) ? 0 : -1;
        }

        void OnSize(UINT, CSize)
        {
            app.resize();
            Invalidate(FALSE);
        }

        void OnPaint(HDC dc)
        {
            app.paint(dc);
        }

        void OnLeftButtonDown(UINT, CPoint point)
        {
            SetCapture();
            app.mouseEvent("mousedown", point.x, point.y);
        }

        void OnLeftButtonUp(UINT, CPoint point)
        {
            ReleaseCapture();
            app.mouseEvent("mouseup", point.x, point.y);
        }

        void OnMouseMove(UINT, CPoint point)
        {
            app.mouseEvent("mousemove", point.x, point.y);
        }

        void OnTimer(UINT_PTR)
        {
            app.timer();
        }

        LRESULT OnCommand(UINT, WPARAM wParam, LPARAM lParam, BOOL&)
        {
            app.command(wParam, lParam);
            return 0;
        }
    };
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand)
{
    if (FAILED(_Module.Init(nullptr, instance)))
        return 1;

    MainFrame frame;
    if (frame.CreateEx() == nullptr)
    {
        _Module.Term();
        return 1;
    }
    frame.ShowWindow(showCommand);
    frame.UpdateWindow();

    CMessageLoop loop;
    _Module.AddMessageLoop(&loop);
    const int result = loop.Run();
    _Module.RemoveMessageLoop();
    _Module.Term();
    return result;
}
