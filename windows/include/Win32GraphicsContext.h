#pragma once

#include <Windows.h>
#include <map>
#include <vector>

#include "SPGraphicsContext.h"

namespace space
{
    class Win32GraphicsContext final : public SPGraphicsContext
    {
    public:
        explicit Win32GraphicsContext(HWND window);
        void setDeviceContext(HDC dc);
        bool handleCommand(WPARAM wParam, LPARAM lParam);

        void setControlAttribute(SPElement *element, const string& name, const string& value) override;
        void request(const string& method, const string& url, const string& data, RequestCallback callback) override;
        void drawImagePart(SPImage *image, int x, int y, int width, int height, int x1, int y1, int x2, int y2) override;
        void drawSkinImage(const string& resourceId, int x, int y, int width, int height) override;
        void setClip(const sp_rectangle& rect) override;
        void declipRect() override;
        void drawLine(int x1, int y1, int x2, int y2, SPColor color) override;
        void drawRectangle(int x, int y, int width, int height, SPColor color) override;
        void fillRectangle(int x, int y, int width, int height, SPColor color) override;
        void fillRoundedRectangle(int x, int y, int width, int height, int radius, SPColor color) override;
        void drawRoundedRectangle(int x, int y, int width, int height, int radius, SPColor color) override;
        void fillRoundedLinearGradientRectangle(int x, int y, int width, int height, int radius, int direction, const SPGradient& gradient) override;
        void drawImage(SPImage *image, int x, int y, int width, int height) override;
        void drawControl(const string& name, SPElement *element, int x, int y, int width, int height) override;
        void drawString(const string& text, SPFontStyle font, SPColor color, int x, int y, int width, int height) override;
        void invalidateRegion(sp_rectangle region) override;
        void drawHTMLText(const string& html, SPFontStyle font, SPColor color, int x, int y, int width, int height) override;
        void drawLinearGradient(const sp_rectangle& rect, const SPGradient& gradient) override;
        SPImage *loadImage(const string& bitmap) override;
        void scheduleThaw(int milliseconds) override;
        void freeze() override;
        void thaw() override;
        sp_rectangle measureString(const string& text, const SPFontStyle& font) override;
        int getWidth() override;
        int getHeight() override;
        void drawInputElement(SPElement *element, int x, int y, int width, int height) override;
        void setOrigo(const int& x, const int& y) override;
        void restoreOrigo() override;

    private:
        HWND window;
        HDC dc = nullptr;
        int savedDc = 0;
        std::vector<int> savedClipStates;
        POINT origin{0, 0};
        bool frozen = false;
        std::map<HWND, SPElement *> inputElements;
    };
}
