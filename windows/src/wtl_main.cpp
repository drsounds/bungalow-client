#include <atlbase.h>
#include <atlapp.h>
#include <atlcrack.h>
#include <atlframe.h>
#include <atltypes.h>

#include "NativeApplication.h"

#include <windowsx.h>

#include <algorithm>
namespace WTL {
using ::CPoint;
using ::CSize;
} // namespace WTL

using namespace ATL;
using namespace WTL;
CAppModule _Module;

namespace {
constexpr int FileMenuButtonId = 1001;
constexpr int MinimizeButtonId = 1002;
constexpr int MaximizeButtonId = 1003;
constexpr int CloseButtonId = 1004;
constexpr int FileAboutCommandId = 1101;
constexpr int FileExitCommandId = 1102;
class MainFrame : public CFrameWindowImpl<MainFrame> {
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("BungalowNativeWtlWindow"), 0, CS_DROPSHADOW,
							   COLOR_WINDOW)

	space::NativeApplication app;

	bool didFailInitialization() const { return initializationFailed; }

	~MainFrame() {
		if (fileMenu)
			DestroyMenu(fileMenu);
	}

	BEGIN_MSG_MAP(MainFrame)
	MSG_WM_CREATE(OnCreate)
	MSG_WM_SIZE(OnSize)
	MESSAGE_HANDLER(WM_PAINT, OnPaint)
	MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
	MESSAGE_HANDLER(WM_NCCALCSIZE, OnNcCalcSize)
	MESSAGE_HANDLER(WM_NCHITTEST, OnNcHitTest)
	MESSAGE_HANDLER(WM_GETMINMAXINFO, OnGetMinMaxInfo)
	MESSAGE_HANDLER(WM_COMMAND, OnCommand)
	MSG_WM_LBUTTONDOWN(OnLeftButtonDown)
	MSG_WM_LBUTTONUP(OnLeftButtonUp)
	MSG_WM_RBUTTONDOWN(OnRightButtonDown)
	MSG_WM_RBUTTONUP(OnRightButtonUp)
	MSG_WM_MOUSEMOVE(OnMouseMove)
	MSG_WM_KEYDOWN(OnKeyDown)
	MSG_WM_TIMER(OnTimer)
	MSG_WM_DESTROY(OnDestroy)
	CHAIN_MSG_MAP(CFrameWindowImpl<MainFrame>)
	END_MSG_MAP()

	LRESULT OnCreate(LPCREATESTRUCT) {
		if (!app.initialize(m_hWnd)) {
			initializationFailed = true;
			return -1;
		}

		fileMenuButton = CreateHeaderButton(L"&File", FileMenuButtonId);
		minimizeButton = CreateHeaderButton(L"\u2014", MinimizeButtonId);
		maximizeButton = CreateHeaderButton(L"\u25A1", MaximizeButtonId);
		closeButton = CreateHeaderButton(L"\u00D7", CloseButtonId);
		fileMenu = CreatePopupMenu();
		if (!fileMenuButton || !minimizeButton || !maximizeButton ||
			!closeButton || !fileMenu) {
			initializationFailed = true;
			return -1;
		}

		AppendMenuW(fileMenu, MF_STRING, FileAboutCommandId, L"&About Spacify");
		AppendMenuW(fileMenu, MF_SEPARATOR, 0, nullptr);
		AppendMenuW(fileMenu, MF_STRING, FileExitCommandId, L"E&xit");
		PositionHeaderButtons();
		return 0;
	}

	void OnSize(UINT, CSize) {
		app.resize();
		PositionHeaderButtons();
		UpdateWindowRegion();
		Invalidate(FALSE);
	}

	LRESULT OnPaint(UINT, WPARAM, LPARAM, BOOL &) {
		PAINTSTRUCT paint{};
		HDC windowDc = ::BeginPaint(m_hWnd, &paint);
		RECT client{};
		GetClientRect(&client);

		// Draw off-screen and blit once to avoid flicker (like wxAutoBufferedPaintDC).
		HDC dc = CreateCompatibleDC(windowDc);
		HBITMAP buffer =
			CreateCompatibleBitmap(windowDc, (std::max)(1L, client.right),
								   (std::max)(1L, client.bottom));
		HGDIOBJ oldBuffer = SelectObject(dc, buffer);
		HBRUSH clear = CreateSolidBrush(RGB(38, 43, 54));
		FillRect(dc, &client, clear);
		DeleteObject(clear);
		IntersectClipRect(dc, paint.rcPaint.left, paint.rcPaint.top,
						  paint.rcPaint.right, paint.rcPaint.bottom);
		app.paint(dc);
		RECT header{0, 0, client.right, space::NativeApplication::HeaderHeight};
		HBRUSH background = CreateSolidBrush(RGB(38, 43, 54));
		FillRect(dc, &header, background);
		DeleteObject(background);

		HPEN separator = CreatePen(PS_SOLID, 1, RGB(54, 60, 72));
		HGDIOBJ oldPen = SelectObject(dc, separator);
		MoveToEx(dc, 0, header.bottom - 1, nullptr);
		LineTo(dc, client.right, header.bottom - 1);
		SelectObject(dc, oldPen);
		DeleteObject(separator);

		SetBkMode(dc, TRANSPARENT);
		SetTextColor(dc, RGB(238, 240, 245));
		RECT title = header;
		DrawTextW(dc, L"Spacify", -1, &title,
				  DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		SelectClipRgn(dc, nullptr);
		BitBlt(windowDc, paint.rcPaint.left, paint.rcPaint.top,
			   paint.rcPaint.right - paint.rcPaint.left,
			   paint.rcPaint.bottom - paint.rcPaint.top, dc, paint.rcPaint.left,
			   paint.rcPaint.top, SRCCOPY);
		SelectObject(dc, oldBuffer);
		DeleteObject(buffer);
		DeleteDC(dc);
		::EndPaint(m_hWnd, &paint);
		return 0;
	}

	LRESULT OnEraseBackground(UINT, WPARAM, LPARAM, BOOL &) { return 1; }

	LRESULT OnNcCalcSize(UINT, WPARAM wParam, LPARAM lParam, BOOL &) {
		if (wParam && IsZoomed()) {
			auto *parameters = reinterpret_cast<NCCALCSIZE_PARAMS *>(lParam);
			MONITORINFO monitor{sizeof(monitor)};
			if (GetMonitorInfoW(
					MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST),
					&monitor))
				parameters->rgrc[0] = monitor.rcWork;
		}
		return 0;
	}

	LRESULT OnNcHitTest(UINT, WPARAM, LPARAM lParam, BOOL &) {
		const POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
		if (IsZoomed()) {
			POINT clientPoint = point;
			ScreenToClient(&clientPoint);
			if (clientPoint.y < space::NativeApplication::HeaderHeight &&
				!IsHeaderButtonAt(clientPoint))
				return HTCAPTION;
			return HTCLIENT;
		}

		RECT window{};
		GetWindowRect(&window);
		const int border = GetSystemMetrics(SM_CXSIZEFRAME) +
						   GetSystemMetrics(SM_CXPADDEDBORDER);
		const bool left = point.x < window.left + border;
		const bool right = point.x >= window.right - border;
		const bool top = point.y < window.top + border;
		const bool bottom = point.y >= window.bottom - border;

		if (top && left)
			return HTTOPLEFT;
		if (top && right)
			return HTTOPRIGHT;
		if (bottom && left)
			return HTBOTTOMLEFT;
		if (bottom && right)
			return HTBOTTOMRIGHT;
		if (left)
			return HTLEFT;
		if (right)
			return HTRIGHT;
		if (top)
			return HTTOP;
		if (bottom)
			return HTBOTTOM;

		POINT clientPoint = point;
		ScreenToClient(&clientPoint);
		if (clientPoint.y < space::NativeApplication::HeaderHeight &&
			!IsHeaderButtonAt(clientPoint))
			return HTCAPTION;
		return HTCLIENT;
	}

	LRESULT OnGetMinMaxInfo(UINT, WPARAM, LPARAM lParam, BOOL &) {
		auto *info = reinterpret_cast<MINMAXINFO *>(lParam);
		info->ptMinTrackSize.x = 640;
		info->ptMinTrackSize.y = 420;
		return 0;
	}

	void OnLeftButtonDown(UINT, CPoint point) {
		SetFocus();
		SetCapture();
		app.mouseEvent("mousedown", point.x, point.y, 0, IsCommandDown());
	}

	void OnLeftButtonUp(UINT, CPoint point) {
		if (GetCapture() == m_hWnd)
			ReleaseCapture();
		app.mouseEvent("mouseup", point.x, point.y, 0, IsCommandDown());
	}

	void OnMouseMove(UINT, CPoint point) {
		app.mouseEvent("mousemove", point.x, point.y, 0, IsCommandDown());
	}

	void OnRightButtonDown(UINT, CPoint point) {
		SetCapture();
		app.mouseEvent("mousedown", point.x, point.y, 1, IsCommandDown());
	}

	void OnRightButtonUp(UINT, CPoint point) {
		if (GetCapture() == m_hWnd)
			ReleaseCapture();
		app.mouseEvent("mouseup", point.x, point.y, 1, IsCommandDown());
	}

	static bool IsCommandDown() { return (GetKeyState(VK_CONTROL) & 0x8000) != 0; }

	void OnKeyDown(UINT keyCode, UINT, UINT) {
		app.keyEvent(static_cast<int>(keyCode), IsCommandDown());
	}

	void OnTimer(UINT_PTR) { app.timer(); }

	void OnDestroy() {
		SetMsgHandled(FALSE);
		PostQuitMessage(0);
	}

	LRESULT OnCommand(UINT, WPARAM wParam, LPARAM lParam, BOOL &) {
		if (HIWORD(wParam) == BN_CLICKED) {
			switch (LOWORD(wParam)) {
			case FileMenuButtonId:
				ShowFileMenu();
				return 0;
			case MinimizeButtonId:
				ShowWindow(SW_MINIMIZE);
				return 0;
			case MaximizeButtonId:
				ShowWindow(IsZoomed() ? SW_RESTORE : SW_MAXIMIZE);
				return 0;
			case CloseButtonId:
				PostMessage(WM_CLOSE);
				return 0;
			}
		}
		switch (LOWORD(wParam)) {
		case FileAboutCommandId:
			::MessageBoxW(m_hWnd, L"Spacify Client", L"About Spacify",
						  MB_OK | MB_ICONINFORMATION);
			return 0;
		case FileExitCommandId:
			PostMessage(WM_CLOSE);
			return 0;
		default:
			app.command(wParam, lParam);
			return 0;
		}
	}

private:
	HWND fileMenuButton = nullptr;
	HWND minimizeButton = nullptr;
	HWND maximizeButton = nullptr;
	HWND closeButton = nullptr;
	HMENU fileMenu = nullptr;
	bool initializationFailed = false;

	HWND CreateHeaderButton(const wchar_t *text, int id) {
		return CreateWindowExW(
			0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_FLAT, 0,
			0, 0, 0, m_hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
			_Module.GetModuleInstance(), nullptr);
	}

	void PositionHeaderButtons() {
		if (!fileMenuButton)
			return;
		RECT client{};
		GetClientRect(&client);
		const int headerHeight = space::NativeApplication::HeaderHeight;
		const int buttonHeight = headerHeight - 8;
		const int buttonWidth = 44;
		::MoveWindow(fileMenuButton, 8, 4, 56, buttonHeight, TRUE);
		::MoveWindow(closeButton, client.right - buttonWidth - 4, 4, buttonWidth,
					 buttonHeight, TRUE);
		::MoveWindow(maximizeButton, client.right - buttonWidth * 2 - 4, 4,
					 buttonWidth, buttonHeight, TRUE);
		::MoveWindow(minimizeButton, client.right - buttonWidth * 3 - 4, 4,
					 buttonWidth, buttonHeight, TRUE);
	}

	bool IsHeaderButtonAt(POINT point) const {
		const HWND buttons[] = {fileMenuButton, minimizeButton, maximizeButton,
								closeButton};
		for (HWND button : buttons) {
			if (!button)
				continue;
			RECT bounds{};
			::GetWindowRect(button, &bounds);
			POINT topLeft{bounds.left, bounds.top};
			POINT bottomRight{bounds.right, bounds.bottom};
			ScreenToClient(&topLeft);
			ScreenToClient(&bottomRight);
			RECT clientBounds{topLeft.x, topLeft.y, bottomRight.x,
							  bottomRight.y};
			if (PtInRect(&clientBounds, point))
				return true;
		}
		return false;
	}

	void ShowFileMenu() {
		RECT button{};
		::GetWindowRect(fileMenuButton, &button);
		TrackPopupMenu(fileMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
					   button.left, button.bottom, 0, m_hWnd, nullptr);
	}

	void UpdateWindowRegion() {
		if (IsZoomed()) {
			::SetWindowRgn(m_hWnd, nullptr, FALSE);
			return;
		}

		RECT client{};
		GetClientRect(&client);
		const int width = client.right;
		const int height = client.bottom;
		const int radius = 12;
		if (width <= 0 || height <= 0)
			return;

		HRGN region = CreateRoundRectRgn(0, 0, width + 1, radius * 2 + 1,
										 radius * 2, radius * 2);
		HRGN body = CreateRectRgn(0, radius, width + 1, height + 1);
		if (!region || !body) {
			if (region)
				DeleteObject(region);
			if (body)
				DeleteObject(body);
			return;
		}
		CombineRgn(region, region, body, RGN_OR);
		DeleteObject(body);
		if (!::SetWindowRgn(m_hWnd, region, TRUE))
			DeleteObject(region);
	}
};
} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
	if (FAILED(_Module.Init(nullptr, instance))) {
		::MessageBoxW(nullptr, L"Unable to initialize the WTL application.",
					  L"Spacify", MB_OK | MB_ICONERROR);
		return 1;
	}

	CMessageLoop loop;
	_Module.AddMessageLoop(&loop);
	int result = 1;
	{
		MainFrame frame;
		const DWORD style = WS_POPUP | WS_THICKFRAME | WS_SYSMENU |
							WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CLIPCHILDREN |
							WS_CLIPSIBLINGS;
		if (frame.CreateEx(nullptr, nullptr, style, WS_EX_APPWINDOW) ==
			nullptr) {
			::MessageBoxW(
				nullptr,
				frame.didFailInitialization()
					? L"Unable to initialize the Spacify user interface."
					: L"Unable to create the Spacify window.",
				L"Spacify", MB_OK | MB_ICONERROR);
		} else {
			::SetWindowTextW(frame.m_hWnd, L"Spacify");
			frame.ShowWindow(showCommand);
			frame.UpdateWindow();
			result = loop.Run();
		}
	}
	_Module.RemoveMessageLoop();
	_Module.Term();
	return result;
}
