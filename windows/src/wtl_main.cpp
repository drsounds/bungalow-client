#include <atlbase.h>
#include <atlapp.h>
#include <atlcrack.h>
#include <atlframe.h>
#include <atltypes.h>

#include "NativeApplication.h"

#ifdef BUNGALOW_CEF
#include "cefhost/CefRuntime.h"
#endif

#include <windowsx.h>

#include <algorithm>
#include <cwctype>
namespace WTL {
using ::CPoint;
using ::CSize;
} // namespace WTL

using namespace ATL;
using namespace WTL;
CAppModule _Module;

namespace {
constexpr int FileAboutCommandId = 1101;
constexpr int FileExitCommandId = 1102;
constexpr int DevelopShowInspectorCommandId = 1201;
constexpr int DevelopReloadPageCommandId = 1202;
constexpr int CaptionButtonWidth = 46;

enum class CaptionItem { None, File, Develop, Minimize, Maximize, Close };

constexpr CaptionItem CaptionItems[] = {CaptionItem::File, CaptionItem::Develop, CaptionItem::Minimize,
										CaptionItem::Maximize, CaptionItem::Close};

bool IsMenuItem(CaptionItem item) {
	return item == CaptionItem::File || item == CaptionItem::Develop;
}

const wchar_t *MenuLabel(CaptionItem item) {
	return item == CaptionItem::Develop ? L"Develop" : L"File";
}

// 80% of the work area of the monitor under the cursor, centered on it.
RECT InitialWindowBounds() {
	POINT cursor{};
	::GetCursorPos(&cursor);
	MONITORINFO info{sizeof(info)};
	::GetMonitorInfoW(::MonitorFromPoint(cursor, MONITOR_DEFAULTTOPRIMARY),
					  &info);
	const RECT &work = info.rcWork;
	const LONG width = (work.right - work.left) * 8 / 10;
	const LONG height = (work.bottom - work.top) * 8 / 10;
	const LONG left = work.left + (work.right - work.left - width) / 2;
	const LONG top = work.top + (work.bottom - work.top - height) / 2;
	return RECT{left, top, left + width, top + height};
}

// Returns the installed font's own LOGFONT so its charset matches; icon fonts are
// otherwise substituted by the font mapper.
bool FindFontFamily(const wchar_t *name, LOGFONTW &result) {
	LOGFONTW query{};
	query.lfCharSet = DEFAULT_CHARSET;
	wcscpy_s(query.lfFaceName, name);
	result = {};
	HDC screen = ::GetDC(nullptr);
	EnumFontFamiliesExW(
		screen, &query,
		[](const LOGFONTW *font, const TEXTMETRICW *, DWORD, LPARAM found) -> int {
			*reinterpret_cast<LOGFONTW *>(found) = *font;
			return 0;
		},
		reinterpret_cast<LPARAM>(&result), 0);
	::ReleaseDC(nullptr, screen);
	return result.lfFaceName[0] != L'\0';
}

// Blends a translucent colour over the buffer (hover/pressed states on top of app content).
void FillTranslucent(HDC dc, const RECT &rect, COLORREF color, BYTE alpha) {
	BITMAPINFO info{};
	info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	info.bmiHeader.biWidth = 1;
	info.bmiHeader.biHeight = 1;
	info.bmiHeader.biPlanes = 1;
	info.bmiHeader.biBitCount = 32;
	info.bmiHeader.biCompression = BI_RGB;
	void *bits = nullptr;
	HBITMAP pixel = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
	if (!pixel || !bits) {
		if (pixel)
			DeleteObject(pixel);
		return;
	}
	auto *bgra = static_cast<BYTE *>(bits);
	bgra[0] = static_cast<BYTE>(GetBValue(color) * alpha / 255);
	bgra[1] = static_cast<BYTE>(GetGValue(color) * alpha / 255);
	bgra[2] = static_cast<BYTE>(GetRValue(color) * alpha / 255);
	bgra[3] = alpha;
	HDC source = CreateCompatibleDC(dc);
	HGDIOBJ old = SelectObject(source, pixel);
	const BLENDFUNCTION blend{AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
	AlphaBlend(dc, rect.left, rect.top, rect.right - rect.left,
			   rect.bottom - rect.top, source, 0, 0, 1, 1, blend);
	SelectObject(source, old);
	DeleteDC(source);
	DeleteObject(pixel);
}

class MainFrame : public CFrameWindowImpl<MainFrame> {
public:
	// Keep WTL's default class styles; CS_DROPSHADOW only shows when the system
	// "Show shadows under windows" setting (SPI_GETDROPSHADOW) is on.
	DECLARE_FRAME_WND_CLASS_EX(_T("BungalowNativeWtlWindow"), 0,
							   CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_DROPSHADOW,
							   COLOR_WINDOW)

	space::NativeApplication app;

	bool didFailInitialization() const { return initializationFailed; }

	~MainFrame() {
		for (HMENU menu : {fileMenu, developMenu})
			if (menu)
				DestroyMenu(menu);
		DestroyCaptionFonts();
	}

	BEGIN_MSG_MAP(MainFrame)
	MSG_WM_CREATE(OnCreate)
	MSG_WM_SIZE(OnSize)
	MESSAGE_HANDLER(WM_PAINT, OnPaint)
	MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
	MESSAGE_HANDLER(WM_NCCALCSIZE, OnNcCalcSize)
	MESSAGE_HANDLER(WM_NCHITTEST, OnNcHitTest)
	MESSAGE_HANDLER(WM_NCACTIVATE, OnNcActivate)
	MESSAGE_HANDLER(WM_NCPAINT, OnNcPaint)
	MESSAGE_HANDLER(WM_GETMINMAXINFO, OnGetMinMaxInfo)
	MESSAGE_HANDLER(WM_COMMAND, OnCommand)
	MESSAGE_HANDLER(WM_SYSCOMMAND, OnSysCommand)
	MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChange)
	MESSAGE_HANDLER(WM_MOUSELEAVE, OnMouseLeave)
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

		fileMenu = CreatePopupMenu();
		developMenu = CreatePopupMenu();
		if (!fileMenu || !developMenu) {
			initializationFailed = true;
			return -1;
		}
		AppendMenuW(fileMenu, MF_STRING, FileAboutCommandId, L"&About Spacify");
		AppendMenuW(fileMenu, MF_SEPARATOR, 0, nullptr);
		AppendMenuW(fileMenu, MF_STRING, FileExitCommandId, L"E&xit");
		AppendMenuW(developMenu, MF_STRING, DevelopShowInspectorCommandId, L"Show &Inspector");
		AppendMenuW(developMenu, MF_STRING, DevelopReloadPageCommandId, L"&Reload Page");
		CreateCaptionFonts();
		return 0;
	}

	void OnSize(UINT, CSize) {
		app.resize();
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
		PaintCaption(dc, client);

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

	// SetWindowRgn disables the DWM frame, so DefWindowProc would paint the
	// classic 3D border over the client area when focus changes. lParam = -1
	// keeps the activation bookkeeping but skips the non-client repaint.
	LRESULT OnNcActivate(UINT message, WPARAM wParam, LPARAM, BOOL &) {
		const LRESULT result = DefWindowProc(message, wParam, -1);
		Invalidate(FALSE);
		return result;
	}

	// The client area covers the whole window; there is no frame to paint.
	LRESULT OnNcPaint(UINT, WPARAM, LPARAM, BOOL &) { return 0; }

	LRESULT OnNcHitTest(UINT, WPARAM, LPARAM lParam, BOOL &) {
		const POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
		POINT clientPoint = point;
		ScreenToClient(&clientPoint);

		if (!IsZoomed()) {
			RECT window{};
			GetWindowRect(&window);
			const int border = GetSystemMetrics(SM_CXSIZEFRAME) +
							   GetSystemMetrics(SM_CXPADDEDBORDER);
			const bool left = point.x < window.left + border;
			const bool right = point.x >= window.right - border;
			const bool top = point.y < window.top + border;
			const bool bottom = point.y >= window.bottom - border;

			// The close button reaches the top-right corner; keep it clickable there.
			if (!(top && right && CaptionItemAt(clientPoint) == CaptionItem::Close)) {
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
			}
		}

		if (clientPoint.y < space::NativeApplication::CaptionHeight &&
			CaptionItemAt(clientPoint) == CaptionItem::None)
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
		const CaptionItem item = CaptionItemAt(point);
		if (IsMenuItem(item)) {
			ShowCaptionMenu(item);
			return;
		}
		if (item != CaptionItem::None) {
			SetPressedItem(item);
			SetCapture();
			return;
		}
		SetFocus();
		SetCapture();
		app.mouseEvent("mousedown", point.x, point.y, 0, IsCommandDown());
	}

	void OnLeftButtonUp(UINT, CPoint point) {
		if (GetCapture() == m_hWnd)
			ReleaseCapture();
		if (pressedItem != CaptionItem::None) {
			const CaptionItem pressed = pressedItem;
			SetPressedItem(CaptionItem::None);
			if (CaptionItemAt(point) == pressed)
				ActivateCaptionItem(pressed);
			return;
		}
		app.mouseEvent("mouseup", point.x, point.y, 0, IsCommandDown());
	}

	void OnMouseMove(UINT, CPoint point) {
		const CaptionItem item = CaptionItemAt(point);
		SetHoveredItem(item);
		if (item != CaptionItem::None || pressedItem != CaptionItem::None)
			return;
		app.mouseEvent("mousemove", point.x, point.y, 0, IsCommandDown());
	}

	LRESULT OnMouseLeave(UINT, WPARAM, LPARAM, BOOL &) {
		trackingMouse = false;
		SetHoveredItem(CaptionItem::None);
		return 0;
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
		switch (LOWORD(wParam)) {
		case FileAboutCommandId:
			::MessageBoxW(m_hWnd, L"Spacify Client", L"About Spacify",
						  MB_OK | MB_ICONINFORMATION);
			return 0;
		case FileExitCommandId:
			PostMessage(WM_CLOSE);
			return 0;
		case DevelopShowInspectorCommandId:
			app.showWebInspector();
			return 0;
		case DevelopReloadPageCommandId:
			app.reloadWebPage();
			return 0;
		default:
			app.command(wParam, lParam);
			return 0;
		}
	}

	// Alt+F / Alt+D open the File / Develop menus, like a native menu bar.
	LRESULT OnSysCommand(UINT, WPARAM wParam, LPARAM lParam, BOOL &handled) {
		if ((wParam & 0xFFF0) == SC_KEYMENU) {
			switch (std::towlower(static_cast<wint_t>(lParam))) {
			case L'f':
				ShowCaptionMenu(CaptionItem::File);
				return 0;
			case L'd':
				ShowCaptionMenu(CaptionItem::Develop);
				return 0;
			}
		}
		handled = FALSE;
		return 0;
	}

	LRESULT OnSettingChange(UINT, WPARAM, LPARAM, BOOL &handled) {
		CreateCaptionFonts();
		Invalidate(FALSE);
		handled = FALSE;
		return 0;
	}

private:
	HMENU fileMenu = nullptr;
	HMENU developMenu = nullptr;
	HFONT titleFont = nullptr;
	HFONT menuFont = nullptr;
	HFONT glyphFont = nullptr;
	bool hasGlyphFont = false;
	CaptionItem hoveredItem = CaptionItem::None;
	CaptionItem pressedItem = CaptionItem::None;
	CaptionItem openMenu = CaptionItem::None;
	bool trackingMouse = false;
	bool initializationFailed = false;

	void CreateCaptionFonts() {
		DestroyCaptionFonts();
		// Use the same fonts Windows uses for real title bars and menus.
		NONCLIENTMETRICSW metrics{sizeof(metrics)};
		if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0)) {
			titleFont = CreateFontIndirectW(&metrics.lfCaptionFont);
			menuFont = CreateFontIndirectW(&metrics.lfMenuFont);
		}
		LOGFONTW icons{};
		hasGlyphFont = FindFontFamily(L"Segoe Fluent Icons", icons) ||
					   FindFontFamily(L"Segoe MDL2 Assets", icons);
		if (hasGlyphFont) {
			icons.lfHeight = -10;
			icons.lfWidth = 0;
			icons.lfWeight = FW_NORMAL;
			icons.lfQuality = CLEARTYPE_QUALITY;
			glyphFont = CreateFontIndirectW(&icons);
			hasGlyphFont = glyphFont != nullptr;
		}
	}

	void DestroyCaptionFonts() {
		for (HFONT *font : {&titleFont, &menuFont, &glyphFont}) {
			if (*font)
				DeleteObject(*font);
			*font = nullptr;
		}
	}

	RECT CaptionItemRect(CaptionItem item) const {
		RECT client{};
		::GetClientRect(m_hWnd, &client);
		const int height = space::NativeApplication::CaptionHeight;
		switch (item) {
		case CaptionItem::File:
			return {4, 0, 4 + MenuLabelWidth(item) + 16, height};
		case CaptionItem::Develop: {
			const LONG left = CaptionItemRect(CaptionItem::File).right;
			return {left, 0, left + MenuLabelWidth(item) + 16, height};
		}
		case CaptionItem::Close:
			return {client.right - CaptionButtonWidth, 0, client.right, height};
		case CaptionItem::Maximize:
			return {client.right - CaptionButtonWidth * 2, 0, client.right - CaptionButtonWidth, height};
		case CaptionItem::Minimize:
			return {client.right - CaptionButtonWidth * 3, 0, client.right - CaptionButtonWidth * 2, height};
		default:
			return {};
		}
	}

	int MenuLabelWidth(CaptionItem item) const {
		int textWidth = 24;
		if (HDC screen = ::GetDC(m_hWnd)) {
			HGDIOBJ old = SelectObject(screen, menuFont ? menuFont : GetStockObject(DEFAULT_GUI_FONT));
			const wchar_t *label = MenuLabel(item);
			SIZE size{};
			if (GetTextExtentPoint32W(screen, label, static_cast<int>(wcslen(label)), &size))
				textWidth = size.cx;
			SelectObject(screen, old);
			::ReleaseDC(m_hWnd, screen);
		}
		return textWidth;
	}

	CaptionItem CaptionItemAt(POINT point) const {
		if (point.y < 0 || point.y >= space::NativeApplication::CaptionHeight)
			return CaptionItem::None;
		for (CaptionItem item : CaptionItems) {
			const RECT rect = CaptionItemRect(item);
			if (PtInRect(&rect, point))
				return item;
		}
		return CaptionItem::None;
	}

	void InvalidateCaption() {
		RECT client{};
		GetClientRect(&client);
		RECT caption{0, 0, client.right, space::NativeApplication::CaptionHeight};
		InvalidateRect(&caption, FALSE);
	}

	void SetHoveredItem(CaptionItem item) {
		if (item != CaptionItem::None && !trackingMouse) {
			TRACKMOUSEEVENT track{sizeof(track), TME_LEAVE, m_hWnd, 0};
			trackingMouse = TrackMouseEvent(&track) != FALSE;
		}
		if (item == hoveredItem)
			return;
		hoveredItem = item;
		InvalidateCaption();
	}

	void SetPressedItem(CaptionItem item) {
		if (item == pressedItem)
			return;
		pressedItem = item;
		InvalidateCaption();
	}

	void ActivateCaptionItem(CaptionItem item) {
		switch (item) {
		case CaptionItem::File:
		case CaptionItem::Develop:
			ShowCaptionMenu(item);
			break;
		case CaptionItem::Minimize:
			ShowWindow(SW_MINIMIZE);
			break;
		case CaptionItem::Maximize:
			ShowWindow(IsZoomed() ? SW_RESTORE : SW_MAXIMIZE);
			break;
		case CaptionItem::Close:
			PostMessage(WM_CLOSE);
			break;
		default:
			break;
		}
	}

	// Picks light or dark caption text from whatever the app painted underneath.
	COLORREF CaptionTextColor(HDC dc, const RECT &client) const {
		const int y = space::NativeApplication::CaptionHeight / 2;
		int total = 0;
		int samples = 0;
		for (int i = 1; i <= 8; ++i) {
			const COLORREF pixel = GetPixel(dc, client.right * i / 9, y);
			if (pixel == CLR_INVALID)
				continue;
			total += (GetRValue(pixel) * 299 + GetGValue(pixel) * 587 + GetBValue(pixel) * 114) / 1000;
			++samples;
		}
		const bool lightBackground = samples > 0 && total / samples > 150;
		return lightBackground ? RGB(24, 24, 24) : RGB(242, 242, 242);
	}

	void PaintCaption(HDC dc, const RECT &client) {
		const int height = space::NativeApplication::CaptionHeight;
		const COLORREF textColor = CaptionTextColor(dc, client);
		const bool lightText = GetRValue(textColor) > 128;
		SetBkMode(dc, TRANSPARENT);

		// Transparent items: only hover/pressed states draw a translucent wash.
		for (CaptionItem item : CaptionItems) {
			const bool active = item == pressedItem || item == openMenu;
			if (item != hoveredItem && !active)
				continue;
			const RECT rect = CaptionItemRect(item);
			if (item == CaptionItem::Close)
				FillTranslucent(dc, rect, RGB(196, 43, 28), active ? 200 : 255);
			else
				FillTranslucent(dc, rect, lightText ? RGB(255, 255, 255) : RGB(0, 0, 0), active ? 48 : 28);
		}

		HGDIOBJ oldFont = SelectObject(dc, titleFont ? titleFont : GetStockObject(DEFAULT_GUI_FONT));
		SetTextColor(dc, textColor);
		RECT title{0, 0, client.right, height};
		DrawTextW(dc, L"Spacify", -1, &title, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

		SelectObject(dc, menuFont ? menuFont : GetStockObject(DEFAULT_GUI_FONT));
		for (CaptionItem item : {CaptionItem::File, CaptionItem::Develop}) {
			RECT rect = CaptionItemRect(item);
			DrawTextW(dc, MenuLabel(item), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		}

		if (glyphFont)
			SelectObject(dc, glyphFont);
		const bool zoomed = IsZoomed() != FALSE;
		const struct {
			CaptionItem item;
			const wchar_t *glyph;
			const wchar_t *fallback;
		} buttons[] = {
			{CaptionItem::Minimize, L"\xE921", L"\x2014"},
			{CaptionItem::Maximize, zoomed ? L"\xE923" : L"\xE922", zoomed ? L"\x2750" : L"\x25A1"},
			{CaptionItem::Close, L"\xE8BB", L"\x2715"},
		};
		for (const auto &button : buttons) {
			RECT rect = CaptionItemRect(button.item);
			SetTextColor(dc, button.item == CaptionItem::Close && button.item == hoveredItem
								 ? RGB(255, 255, 255) : textColor);
			DrawTextW(dc, hasGlyphFont ? button.glyph : button.fallback, -1, &rect,
					  DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		}
		SelectObject(dc, oldFont);
	}

	void ShowCaptionMenu(CaptionItem item) {
		const RECT rect = CaptionItemRect(item);
		POINT anchor{rect.left, rect.bottom};
		ClientToScreen(&anchor);
		openMenu = item;
		InvalidateCaption();
		TrackPopupMenu(item == CaptionItem::Develop ? developMenu : fileMenu,
					   TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
					   anchor.x, anchor.y, 0, m_hWnd, nullptr);
		openMenu = CaptionItem::None;
		SetHoveredItem(CaptionItem::None);
		InvalidateCaption();
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
#ifdef BUNGALOW_CEF
	// CEF relaunches this executable for its renderer and GPU processes.
	const int subprocessExitCode = space::cefhost::executeSubprocess(instance);
	if (subprocessExitCode >= 0)
		return subprocessExitCode;
	// On failure app bundles show an error view; the rest of the UI still works.
	space::cefhost::initialize(instance);
#endif

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
		RECT bounds = InitialWindowBounds();
		const DWORD style = WS_POPUP | WS_THICKFRAME | WS_SYSMENU |
							WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CLIPCHILDREN |
							WS_CLIPSIBLINGS;
		if (frame.CreateEx(nullptr, bounds, style, WS_EX_APPWINDOW) ==
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
#ifdef BUNGALOW_CEF
	space::cefhost::shutdown();
#endif
	return result;
}
