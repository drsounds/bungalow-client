#ifndef GRAPHICSCONTEXT_H
#define GRAPHICSCONTEXT_H
#include "libspace.h"

#include "SPColor.h"
#include "SPFontStyle.h"

#include <map>

#include "SpaceBase.h"
#include "Space.h"

using namespace std;

namespace space {
	class SPWindowElement;
	class SPColor;
	class SPFontStyle;
	class SPElement;
	class SPImage;
	class slice;
	class SPSkin;
	class Space;
	class SPGraphicsContext {
	protected:
		SPFontStyle *fontStyle{};
    	Space *m_space;
	public:
		virtual ~SPGraphicsContext() = default;
    	SPGraphicsContext();
		explicit SPGraphicsContext(Space *space);
		SPSkin *skin{};
		virtual void setControlAttribute(SPElement *, const string&, const string&) {}
		virtual void drawImagePart(SPImage *image, int x, int y, int width, int height, int x1, int y1, int x2, int y2) {}
		virtual void drawSkinImage(const string& resourceId, int x, int y, int w, int h) {}
		virtual void restoreOrigo() {}
		void drawImageSlices(const map<string, slice *>& slices, string selector, int left, int top, int width, int height);
		virtual void setClip(const sp_rectangle& rect) {}
		virtual void drawLine(int x, int y, int x2, int y2, SPColor color) {}
		virtual void drawRectangle(int x, int y, int w, int h, SPColor color) {}
		virtual void fillRectangle(int x, int y, int w, int h, SPColor color) {}
		virtual void fillRoundedRectangle(int x, int y, int w, int h, int r, SPColor color) {}
		virtual void drawRoundedRectangle(int x, int y, int w, int h, int r, SPColor color) {}
		virtual void fillRoundedLinearGradientRectangle(int x, int y, int w, int h, int r, int dir, const SPGradient& gradient) {}
		virtual void drawImage(SPImage *image, int x, int y, int w, int h) {}
		virtual void drawControl(const string& name, SPElement *elm, int x, int y, int w, int h) {}
		virtual void drawString(const string& text, SPFontStyle fs, SPColor color, int x, int y, int w, int h) { }
		virtual void invalidateRegion(sp_rectangle region) {}
		virtual void drawHTMLText(const string& html, SPFontStyle fs, SPColor color, int x, int y, int w, int h) { }
		virtual void drawLinearGradient(const sp_rectangle& rect, const SPGradient& gradient) {}
		virtual SPImage *loadImage(const string& bitmap) { return NULL; }
		virtual void freeze() {}
		virtual void thaw() {}
		virtual sp_rectangle measureString(const string& text, const SPFontStyle& font) { return sp_rectangle(); }
		virtual int getWidth() { return 0; }
		virtual int getHeight() { return 0; }
		virtual void declipRect() { }
		void setFontStyle(space::SPFontStyle *fs) {
			this->fontStyle = fs;
		}
		SPFontStyle *getFontStyle() const {
			return this->fontStyle;
		}
		virtual void drawInputElement(SPElement *srcElement, int x, int y, int w, int h) { }
		virtual void setOrigo(const int& x, const int& y);
	};

}
#endif // GRAPHICSCONTEXT_H
