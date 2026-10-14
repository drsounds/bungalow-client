#pragma once

#ifndef H_WIN32SPACE
#define H_WIN32SPACE

#include <Windows.h>
#include <wingdi.h>
#include <map>
#include "libspace.h"
#include "SPColor.h"
#include "Element.h"
#include "GraphicsContext.h"
#include "InputEventArgs.h"
#include <map>
using namespace std;
using namespace space;
LRESULT CALLBACK subEditProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam);
namespace space {
	class SPImage;
	class SPElement;
    class WindowElement;
	COLORREF toWin32Color(space::SPColor color);
	class Win32GraphicsContext : public GraphicsContext {
	private:
		HDC hDC;
		HWND hWnd;
	public:
	    LPPOINT currentOffset;
	    void restoreOrigo();
	    HRGN currentRgn;
		static map<SPElement *, HWND> controls;
		static map<HWND, SPElement *> hwnds;
		Win32GraphicsContext(HWND hWnd, HDC hDC, WindowElement *window);
		~Win32GraphicsContext();
        SPImage *loadImage(const string& bitmap);

        virtual void drawImagePart(SPImage *image, int x, int y, int width, int height, int x1, int y1, int x2, int y2);
        virtual void drawSkinImage(string resourceId, int x, int y, int w, int h);
		void setClip(sp_rectangle rect);
		void drawLine(int x1, int y1, int w, int h, space::SPColor *color);
		void drawImage(SPImage *image, int x1, int y1, int w, int h);
		void drawRectangle(int x1, int y1, int w, int h, space::SPColor *color);
		void fillRectangle(int x1, int y1, int w, int h, space::SPColor *color);
        void drawControl(string control, SPElement *elm, int x, int y, int w, int h);
		void invalidateRegion(rectangle region);
		void drawString(string text, space::SPFontStyle *fontStyle, space::SPColor *color,int x, int y, int w, int h);
		void drawInputElement(Element *elm, int x, int y, int w, int h);
		void setFontStyle(space::FontStyle *fs);
		void setOrigo(const int& x, const int& y);
		void drawHTMLText(string html, SPFontStyle *fs, SPColor *color, int x, int y, int w, int h);
		sp_rectangle measureString(string text, space::SPFontStyle *font);
		virtual void declipRect();
	};
};
#endif
