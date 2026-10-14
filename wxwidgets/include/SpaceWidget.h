#ifndef SPACEWIDGET_H
#define SPACEWIDGET_H
#include <wx/wx.h>

#include "SPObject.h"
#include "Space.h"
#include "libspace.h"

enum MySpecialWidgetStyles
{
    SWS_LOOK_CRAZY = 1,
    SWS_LOOK_SERIOUS = 2,
    SWS_SHOW_BUTTON = 4,

    SWS_DEFAULT_STYLE = (SWS_SHOW_BUTTON|SWS_LOOK_SERIOUS)
};
namespace space
{
    class Space;
    class SPWindowElement;
}
class SpaceFrame;
class SpaceWidget : public wxControl
{
    space::Space *m_space;
    SpaceFrame* m_frame;

    int m_mouseX = 0;
    int m_mouseY = 0;
    bool m_leftMouseButtonPressed;

    virtual wxSize DoGetBestSize();

    void OnPaint(wxPaintEvent&);

public:
    void setSpace(space::Space *space) {
        this->m_space = space;
    }
    space::Space *getSpace() {
        return this->m_space;
    }
    wxDECLARE_DYNAMIC_CLASS(SpaceWidget);
	wxSize DoGetBestSize() const;
    void OnMouseMove(wxMouseEvent& event);
    void Init();
    SpaceWidget()
    {
        this->m_space = nullptr;
        this->m_frame = nullptr;
    }
    SpaceWidget(
        wxWindow *parent,
        wxWindowID winId,
        const wxString& label,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = SWS_DEFAULT_STYLE,
        const wxValidator& val = wxDefaultValidator,
        const wxString& name = "MySpecialWidget"
    );

    bool Create(
        wxWindow *parent,
        wxWindowID winId,
        const wxString& label,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = SWS_DEFAULT_STYLE,
        const wxValidator& val = wxDefaultValidator,
        const wxString& name = "Space"
    );
    void OnLeftClick(wxCommandEvent& event);
    void OnRightClick(wxCommandEvent& event);
    void OnLeftUp(wxMouseEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnRightUp(wxMouseEvent& event);
    void OnRightDown(wxMouseEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnSize(wxSizeEvent& event);

    // accessors...
    wxDECLARE_EVENT_TABLE();

};

#endif