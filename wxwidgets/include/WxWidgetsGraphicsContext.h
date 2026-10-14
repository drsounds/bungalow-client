#ifndef SPGRAPHICSCONTEXT_H
#define SPGRAPHICSCONTEXT_H
#include <wx/dc.h>
#include <wx/graphics.h>
#include <wx/dcscreen.h>
#include <wx/dcclient.h>
#include <wx/image.h>
#include <wx/dcprint.h>
#include <wx/wx.h>
#include "SPGraphicsContext.h"
#include <wx/dcbuffer.h>

class wxControlBase;
class SpaceFrame;
class SpaceWidget;

namespace space
{
  class SPImage;
  class SPColor;
  namespace platform
  {
    namespace wxwidgets
    {
      class WxWidgetsGraphicsContext : public SPGraphicsContext
      {
        SpaceFrame *m_frame;
        wxWindow *m_window;
        map<string, wxControlBase *> elements;
        map<wxControlBase *, SPElement *> widgets;
        wxPoint *origo = new wxPoint(0, 0);
        SpaceWidget *m_widget;
        wxAutoBufferedPaintDC* m_paintDc;
      public:
        WxWidgetsGraphicsContext();
        static wxColor toWxColor(SPColor color);
        static SPColor fromWxColor(wxColour color);
        explicit WxWidgetsGraphicsContext(SpaceFrame*);
        void OnTextEnter(wxEvent& evt);
        void setFrame(SpaceFrame *frame)
        {
          this->m_frame = frame;
        }
        SpaceFrame *getFrame() const
        {
          return this->m_frame;
        }

        void setWindow(wxWindow *window)
        {
          this->m_window = window;
        }
        wxWindow *getWindow() const
        {
          return this->m_window;
        }
        ~WxWidgetsGraphicsContext() override;
        SPImage *loadImage(const string& bitmap) override;
        void setControlAttribute(SPElement *, const string&, const string&) override;
        void setControlVisible(SPElement* elm, bool visible);
        void drawImagePart(SPImage *image, int x, int y, int width, int height, int x1, int y1, int x2, int y2) override;
        virtual void drawSkinImage(string resourceId, int x, int y, int w, int h);
        void setClip(const sp_rectangle& rect) override;
        void declipRect() override;
        void drawLine(int x1, int y1, int w, int h, space::SPColor color) override;
        void drawImage(SPImage *image, int x1, int y1, int w, int h) override;
        void drawRectangle(int x, int y, int w, int h, space::SPColor color) override;
        void fillRectangle(int x, int y, int w, int h, space::SPColor color) override;
        void drawControl(const string& control, SPElement *elm, int x, int y, int w, int h) override;
        void invalidateRegion(sp_rectangle region) override;
        void drawString(const string& text, space::SPFontStyle fontStyle, space::SPColor color,int x, int y, int w, int h) override;
        void drawInputElement(SPElement *elm, int x, int y, int w, int h) override;
        void setFontStyle(space::SPFontStyle *fs) const;
        void setOrigo(const int& x, const int& y) override;
        void restoreOrigo() override;
        int getWidth() override;
        int getHeight() override;
        void drawLinearGradient(const sp_rectangle& rect, const SPGradient& gradient) override;
        void drawHTMLText(const string& html, SPFontStyle fontSize, SPColor color, int x, int y, int w, int h) override;
      	void fillRoundedRectangle(int x, int y, int w, int h, int r, SPColor color) override;
      	void drawRoundedRectangle(int x, int y, int w, int h, int r, SPColor color) override;
      	void fillRoundedLinearGradientRectangle(int x, int y, int w, int h, int r, int dir, const SPGradient& gradient) override;
      	sp_rectangle measureString(const string& text, const SPFontStyle& font) override;
        void freeze() override;
        void thaw() override;
        void setPaintDC(wxAutoBufferedPaintDC* dc) {
            m_paintDc = dc;
        }
        wxAutoBufferedPaintDC* getPaintDC() const
        {
            return m_paintDc;
        }

        SpaceWidget* getWidget() const
        {
          return m_widget;
        }
        void setWidget(SpaceWidget* value)
        {
          this->m_widget = value;
        }

      };
    }
  }
}
#endif