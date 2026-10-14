#include "WxWidgetsGraphicsContext.h"
#include <wx/webrequest.h>
#include <wx/fontenum.h>
#include <wx/url.h>
#include <wx/protocol/http.h>
#include <wx/wfstream.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>

#include <filesystem>
#include <functional>
#include <utility>

#include "SPImage.h"
#include "SPInputEventArgs.h"
#include "SPElement.h"
#include "SpaceFrame.h"
#include "SpaceWidget.h"

namespace space
{
    namespace platform
    {
        namespace wxwidgets
        {
            void WxWidgetsGraphicsContext::request(const string& method,
                                                    const string& url,
                                                    const string& data,
                                                    RequestCallback callback)
            {
                wxWindow *window = m_widget ? m_widget->GetParent() : nullptr;
                if (!window) {
                    if (callback)
                        callback(false, {});
                    return;
                }

                const int eventId = wxNewId();
                wxWebRequest request = wxWebSession::GetDefault().CreateRequest(
                    window, wxString::FromUTF8(url.c_str()), eventId);
                request.SetMethod(wxString::FromUTF8(method.c_str()));
                if (method != "get" && method != "HEAD" && method != "DELETE")
                    request.SetData(data.c_str(), "application/json");
                if (!request.IsOk()) {
                    if (callback)
                        callback(false, {});
                    return;
                }

                window->Bind(wxEVT_WEBREQUEST_STATE,
                    [callback = std::move(callback)](wxWebRequestEvent& event) mutable {
                        const auto state = event.GetState();
                        if (state == wxWebRequest::State_Completed) {
                            if (callback)
                                callback(true, event.GetResponse().AsString().ToStdString());
                        } else if (state == wxWebRequest::State_Failed ||
                                   state == wxWebRequest::State_Cancelled) {
                            if (callback)
                                callback(false, {});
                        }
                    }, eventId);
                request.Start();
            }

            static wxString resolveWxFontFamily(const std::string& preferred) {
                wxString face(preferred);
                if (wxFontEnumerator::IsValidFacename(face))
                    return face;
                return wxString("Helvetica");
            }

            WxWidgetsGraphicsContext::WxWidgetsGraphicsContext(SpaceFrame *frame) : WxWidgetsGraphicsContext()
            {
                this->m_frame = frame;
            }

            wxColour WxWidgetsGraphicsContext::toWxColor(SPColor color)
            {
                return wxColour(color.getR(), color.getG(), color.getB(), color.getA());
            }

            SPColor WxWidgetsGraphicsContext::fromWxColor(wxColour color)
            {
                return SPColor(color.GetRed(), color.GetGreen(), color.GetBlue(), color.GetAlpha());
            }

            WxWidgetsGraphicsContext::WxWidgetsGraphicsContext() : m_frame(nullptr), m_window(nullptr),
                                                     m_widget(nullptr),
                                                     m_paintDc(nullptr)
            {
                this->origo = new wxPoint(0, 0);
            }

            WxWidgetsGraphicsContext::~WxWidgetsGraphicsContext()
            {
            }

            // --------------------------------------------------------------
            // Clipping
            // --------------------------------------------------------------


            void WxWidgetsGraphicsContext::setClip(const sp_rectangle& rect)
            {
                this->getPaintDC()->SetClippingRegion(rect.x - this->origo->x, rect.y - this->origo->y, rect.width, rect.height);
            }

            void WxWidgetsGraphicsContext::declipRect()
            {
                this->getPaintDC()->DestroyClippingRegion();
            }

            // --------------------------------------------------------------
            // Transform
            // --------------------------------------------------------------

            void WxWidgetsGraphicsContext::setOrigo(const int& x, const int& y)
            {
                if (this->getPaintDC()->CanUseTransformMatrix())
                {
                    wxAffineMatrix2D matrix{};
                    matrix.Translate(x, y);
                    this->getPaintDC()->SetTransformMatrix(matrix);
                }
                else
                {
                    this->origo->x = x;
                    this->origo->y = y;
                }
            }

            void WxWidgetsGraphicsContext::restoreOrigo()
            {
                if (this->getPaintDC()->CanUseTransformMatrix())
                {
                    this->getPaintDC()->ResetTransformMatrix();
                }
                else
                {
                    this->origo->x = 0;
                    this->origo->y = 0;
                }
            }

            int WxWidgetsGraphicsContext::getWidth()
            {
                if (m_widget == nullptr)
                {
                    return 0;
                }
                return this->m_widget->GetClientSize().GetWidth();
            }

            int WxWidgetsGraphicsContext::getHeight()
            {
                if (m_widget == nullptr)
                {
                    return 0;
                }
                return this->m_widget->GetClientSize().GetHeight();
            }
            void WxWidgetsGraphicsContext::drawHTMLText(const string& html, SPFontStyle fontSize, SPColor color, int x, int y,
                                                        int w, int h)
            {
            }

            void WxWidgetsGraphicsContext::fillRoundedRectangle(int x, int y,
	            int width, int height, int r, SPColor color) {
                const std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(*getPaintDC()));
                if (!gc) return;
                gc->SetBrush(wxBrush(toWxColor(color)));
                gc->SetPen(*wxTRANSPARENT_PEN);
                gc->DrawRoundedRectangle(x - origo->x, y - origo->y, width, height, r);
            }

            void WxWidgetsGraphicsContext::drawRoundedRectangle(int x, int y,
            int width, int height, int r, SPColor color) {
                const std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(*getPaintDC()));
                if (!gc) return;
                gc->SetPen(wxPen(toWxColor(color)));
                gc->SetBrush(*wxTRANSPARENT_BRUSH);
                gc->DrawRoundedRectangle(x - origo->x, y - origo->y, width, height, r);
            }

            void WxWidgetsGraphicsContext::fillRoundedLinearGradientRectangle(
	            int x,
				int y,
				int width,
				int height,
				int r,
				int dir,
	            const SPGradient &gradient
	        ) {
                const std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(*getPaintDC()));
                if (!gc) return;
                wxGraphicsBrush gradientBrush = gc->CreateLinearGradientBrush(
                    x, y, x, y + height, toWxColor(gradient.getStart()), toWxColor(gradient.getEnd()));
                gc->SetBrush(gradientBrush);
                gc->SetPen(*wxTRANSPARENT_PEN);
                gc->DrawRoundedRectangle(x - origo->x, y - origo->y, width, height, r);
            }

            // --------------------------------------------------------------
            // Primitives
            // --------------------------------------------------------------

            void WxWidgetsGraphicsContext::drawLine(int x1, int y1,
                                             int x2, int y2,
                                             SPColor color)
            {
                const std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(*getPaintDC()));
                if (!gc) return;
                gc->SetPen(wxPen(toWxColor(color)));
                gc->StrokeLine(x1 - origo->x, y1 - origo->y, x2 - origo->x, y2 - origo->y);
            }

            void WxWidgetsGraphicsContext::drawRectangle(int x, int y,
                                                     int width, int height,
                                                     SPColor color)
            {
                wxPen pen(wxColour(color.getR(),
                                   color.getG(),
                                   color.getB()), 1);
                this->getPaintDC()->SetPen(pen);
                this->getPaintDC()->SetBrush(*wxTRANSPARENT_BRUSH);

                this->getPaintDC()->DrawRectangle(x - this->origo->x, y - this->origo->y, width - x, height - y);
            }

            void WxWidgetsGraphicsContext::fillRectangle(int x, int y,
                                                     int width, int height,
                                                     SPColor color)
            {
                const std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(*getPaintDC()));
                if (!gc) return;
                gc->SetBrush(wxBrush(toWxColor(color)));
                gc->SetPen(*wxTRANSPARENT_PEN);
                gc->DrawRectangle(x - origo->x, y - origo->y, width, height);
            }

            // --------------------------------------------------------------
            // Imaging
            // --------------------------------------------------------------

            SPImage* WxWidgetsGraphicsContext::loadImage(const std::string& file)
            {
                static auto bitmapDeleter = [](void* h) { delete static_cast<wxBitmap*>(h); };

#ifdef wxHAS_SVG
                // Use wxBitmapBundle for SVG files so they rasterise cleanly at icon size
                if (file.size() > 4 &&
                    file.compare(file.size() - 4, 4, ".svg") == 0)
                {
                    wxBitmapBundle bundle = wxBitmapBundle::FromSVGFile(
                        wxString::FromUTF8(file), wxSize(16, 16));
                    if (bundle.IsOk()) {
                        wxBitmap bmp = bundle.GetBitmap(wxSize(16, 16));
                        if (bmp.IsOk()) {
                            SPImage* img = new SPImage(bmp.GetWidth(), bmp.GetHeight());
                            img->handle = new wxBitmap(bmp);
                            img->handleDeleter = bitmapDeleter;
                            return img;
                        }
                    }
                    return nullptr;
                }
#endif
                wxBitmap bmp(file, wxBITMAP_TYPE_ANY);

                if (!bmp.IsOk())
                    return nullptr;

                SPImage* img = new SPImage(bmp.GetWidth(), bmp.GetHeight());
                img->handle = new wxBitmap(bmp);
                img->handleDeleter = bitmapDeleter;

                return img;
            }

            void WxWidgetsGraphicsContext::drawImage(SPImage* image,
                                                int x, int y,
                                                int width, int height)
            {
                if (!image || !image->handle) return;

                wxBitmap* bmp = static_cast<wxBitmap*>(image->handle);

                // Scale to requested size if needed
                if (bmp->GetWidth() != width || bmp->GetHeight() != height) {
                    wxImage scaled = bmp->ConvertToImage()
                        .Scale(width, height, wxIMAGE_QUALITY_HIGH);
                    this->getPaintDC()->DrawBitmap(
                        wxBitmap(scaled),
                        wxPoint(x - this->origo->x, y - this->origo->y),
                        true);
                } else {
                    this->getPaintDC()->DrawBitmap(
                        *bmp,
                        wxPoint(x - this->origo->x, y - this->origo->y),
                        true);
                }
            }

            void WxWidgetsGraphicsContext::drawImagePart(SPImage* image,
                                                    int left, int top,
                                                    int width, int height,
                                                    int sx, int sy,
                                                    int sw, int sh)
            {
                if (!image || !image->handle) return;

                wxBitmap* bmp = static_cast<wxBitmap*>(image->handle);

                wxBitmap sub = bmp->GetSubBitmap(wxRect(sx, sy, sw, sh));
                this->getPaintDC()->DrawBitmap(sub, wxPoint(left - this->origo->x, top - this->origo->y), true);
            }

            void WxWidgetsGraphicsContext::drawSkinImage(string resourceId, int x, int y, int w, int h)
            {
            }

            void WxWidgetsGraphicsContext::drawLinearGradient(const sp_rectangle& rect, const SPGradient& gradient)
            {
                const std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(*getPaintDC()));
                if (!gc) return;
                wxGraphicsBrush grad = gc->CreateLinearGradientBrush(
                    rect.x, rect.y, rect.x, rect.y + rect.height,
                    toWxColor(gradient.getStart()), toWxColor(gradient.getEnd()));
                gc->SetBrush(grad);
                gc->SetPen(*wxTRANSPARENT_PEN);
                gc->DrawRectangle(rect.x - origo->x, rect.y - origo->y, rect.width, rect.height);
            }

            void WxWidgetsGraphicsContext::freeze() {
                this->m_frame->Freeze();
            }

            void WxWidgetsGraphicsContext::thaw() {
                this->m_frame->Thaw();
            }

            // --------------------------------------------------------------
            // Text
            // --------------------------------------------------------------

            sp_rectangle WxWidgetsGraphicsContext::measureString(const std::string& text,
                                                          const SPFontStyle& fs)
            {
                wxFont font(fs.height,
                            wxFONTFAMILY_DEFAULT,
                            wxFONTSTYLE_NORMAL,
                            wxFONTWEIGHT_NORMAL,
                            false,
                            resolveWxFontFamily(fs.family));
            	wxClientDC dc(this->getWidget());
                dc.SetFont(font);
                wxSize size = dc.GetTextExtent(text);
                return { size.GetWidth(), size.GetHeight(), size.x, size.y };
            }

            void WxWidgetsGraphicsContext::drawString(const std::string& text,
                                               SPFontStyle fs,
                                               SPColor color,
                                               int x, int y,
                                               int w, int h)
            {
                wxColour col(color.getR(), color.getG(), color.getB());

                wxFont font(fs.getHeight(),
                            wxFONTFAMILY_DEFAULT,
                            wxFONTSTYLE_NORMAL,
                            wxFONTWEIGHT_NORMAL,
                            false,
                            resolveWxFontFamily(fs.getFamily()));

                this->getPaintDC()->SetTextForeground(col);
                this->getPaintDC()->DrawText(wxString(text), x - this->origo->x, y - this->origo->y);
            }

            // --------------------------------------------------------------
            // Invalidation
            // --------------------------------------------------------------

            void WxWidgetsGraphicsContext::invalidateRegion(space::sp_rectangle rect)
            {
                if (this->m_window == nullptr) return;
                this->m_window->RefreshRect(wxRect(rect.x - this->origo->x, rect.y - this->origo->y, rect.width, rect.height));
            }

            // --------------------------------------------------------------
            // Controls (text input)
            // --------------------------------------------------------------

            void WxWidgetsGraphicsContext::drawControl(const std::string& control,
                                                SPElement* elm,
                                                int x, int y,
                                                int w, int h)
            {
                // Only supports text controls (“EDIT”)
                if (elements.count(elm->getId()) == 0)
                {
                	int width = elm->getWidth();
                	int height = elm->getHeight();
                    wxTextCtrl* ctrl = new wxTextCtrl(
                        this->m_window,
                        wxID_ANY,
                        "",
                        wxPoint(x, y),
                        wxSize(width, height),
                        wxTE_PROCESS_ENTER
                    );

                    elements[elm->getId()] = ctrl;
                    widgets[ctrl] = elm;

                    ctrl->Bind(
                        wxEVT_COMMAND_TEXT_ENTER,
                        &WxWidgetsGraphicsContext::OnTextEnter,
                        this
                    );
                }
                else
                {
                    wxControlBase *control = elements[elm->getId()];
                    if (control != nullptr) {
                        control->SetSize(x - this->origo->x, y - this->origo->y, w, h);
                    }
                }
            }

            void WxWidgetsGraphicsContext::OnTextEnter(wxEvent& evt)
            {
                wxTextCtrl *widget = (wxTextCtrl *)evt.GetEventObject();
                SPElement *elm = widgets[widget];
                elm->notify(
                    "enter",
                    elm,
                    new SPInputEventArgs(widget->GetValue().utf8_string())
                );
            }
            void WxWidgetsGraphicsContext::setControlVisible(SPElement* elm, bool visible) {
                if (elements.count(elm->getId()) > 0)
                    elements[elm->getId()]->Show(visible);
            }

            void WxWidgetsGraphicsContext::setControlAttribute(SPElement *elm, const string& attrName, const string& value) {
                if (elements.count(elm->getId()) > 0) {
                    wxTextCtrl *widget = (wxTextCtrl*)elements[elm->getId()];
                    if (attrName == "value" || attrName == "text") {
                        widget->SetValue(wxString(value));
                    }
                }
            }
            void WxWidgetsGraphicsContext::drawInputElement(SPElement* elm,
                                                     int x, int y,
                                                     int w, int h)
            {
                auto b = elm->getAbsoluteBounds();
                drawControl("EDIT", elm, b.x - this->origo->x, b.y - this->origo->y, b.width, b.height);
            }

            void WxWidgetsGraphicsContext::setFontStyle(space::SPFontStyle* fs) const
            {
                this->getPaintDC()->SetFont(wxFont(fs->getHeight(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, resolveWxFontFamily(fs->getFamily())));
            }
        }
    }
} // namespace space
