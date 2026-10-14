//
// Created by Alexander Forselius on 2025-12-03.

#include "SpaceWidget.h"
#include "SpaceFrame.h"
#include "SPMouseEventArgs.h"
#include "SPEventArgs.h"
#include "SPWindowElement.h"
#include "WxWidgetsGraphicsContext.h"
#include "Space.h"


BEGIN_EVENT_TABLE(SpaceWidget, wxControl)
    EVT_PAINT(SpaceWidget::OnPaint)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(SpaceWidget, wxControl);

SpaceWidget::SpaceWidget(wxWindow *parent,
                           wxWindowID winId,
                           const wxString& label,
                           const wxPoint& pos,
                           const wxSize& size,
                           long style,
                           const wxValidator& val,
                           const wxString& name ) : wxControl(parent, winId)
{
    this->Init();
    this->Create(parent, winId, label, pos, size, style, val, name);
}


wxSize SpaceWidget::DoGetBestSize() const
{
	// Do not return any arbitrary default value...
	wxASSERT_MSG( m_widget, wxT("myStaticText::DoGetBestSize called before creation") );

	// There is probably a better way than this, but I don't know it.
	// So I use the const_cast to grab a dc that is only used for metrics.
	wxClientDC dc (const_cast<SpaceWidget*> (this));

	wxCoord w = 0;
	wxCoord h = 0;
	dc.GetTextExtent( GetLabel (), &w, &h );

	return { w, h };
}

bool SpaceWidget::Create(wxWindow *parent,
            wxWindowID winId,
            const wxString& label,
            const wxPoint& pos,
            const wxSize& size,
            long style,
            const wxValidator& val,
            const wxString& name )
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, &SpaceWidget::OnPaint, this);
    Bind(wxEVT_SIZE, &SpaceWidget::OnSize, this);
    Bind(wxEVT_LEFT_DOWN,  &SpaceWidget::OnLeftDown,  this);
    Bind(wxEVT_LEFT_UP,   &SpaceWidget::OnLeftUp,    this);
    Bind(wxEVT_RIGHT_DOWN, &SpaceWidget::OnRightDown, this);
    Bind(wxEVT_RIGHT_UP,  &SpaceWidget::OnRightUp,   this);
    Bind(wxEVT_MOTION,    &SpaceWidget::OnMouseMove,  this);
    Bind(wxEVT_COMMAND_LEFT_CLICK, &SpaceWidget::OnLeftClick, this);
    Bind(wxEVT_KEY_DOWN, &SpaceWidget::OnKeyDown, this);
    return FALSE;
}

void SpaceWidget::OnLeftClick(wxCommandEvent& event) {
    auto *args = new space::SPMouseEventArgs(0, this->m_mouseX, this->m_mouseY);
    const space::SPEvent evt("click", args, nullptr);
    this->getSpace()->dispatchEvent(
        "click",
        args,
        this->m_frame->getWindowElement()
    );
}

void SpaceWidget::OnRightClick(wxCommandEvent& event)
{
    auto *args = new space::SPMouseEventArgs(1, this->m_mouseX, this->m_mouseY);
    const space::SPEvent evt("click", args, nullptr);
    this->m_space->dispatchEvent(
        "click",
        args,
        this->m_frame->getWindowElement()
    );
}

void SpaceWidget::OnLeftUp(wxMouseEvent& event) {
    auto *args = new space::SPMouseEventArgs(0, event.GetX(), event.GetY());
    const space::SPEvent evt("mouseup", args, nullptr);
    this->getSpace()->dispatchEvent(
        "mouseup",
        args,
        this->m_frame->getWindowElement()
    );
	if (this->m_leftMouseButtonPressed) {
		auto *args2 = new space::SPMouseEventArgs(1, event.GetX(), event.GetY());
		space::SPEvent evt2("click", args2, nullptr);
		this->getSpace()->dispatchEvent(
			"click",
			args2,
			this->m_frame->getWindowElement()
		);
	}
}

void SpaceWidget::OnRightDown(wxMouseEvent& event)
{
    auto *args = new space::SPMouseEventArgs(1, event.GetX(), event.GetY());
    space::SPEvent evt("mousedown", args, nullptr);
    this->getSpace()->dispatchEvent(
        "mousedown",
        args,
        this->m_frame->getWindowElement()
    );
}

void SpaceWidget::OnSize(wxSizeEvent& event)
{
    //this->m_frame->getWindowElement()->resize(event.GetSize().GetWidth() + 1, event.GetSize().GetHeight() + 1);
    this->m_frame->getWindowElement()->pack();
    this->m_frame->getWindowElement()->setAbsoluteBounds({ 0, 0, event.GetSize().GetWidth(), event.GetSize().GetHeight() });
}

void SpaceWidget::OnLeftDown(wxMouseEvent& event) {
    SetFocus(); // grab keyboard focus so key events are delivered here
    auto pos = event.GetPosition();
    auto *args = new space::SPMouseEventArgs(0, pos.x, pos.y, event.CmdDown());
    space::SPEvent evt("mousedown", args, nullptr);
    this->m_leftMouseButtonPressed = true;
    this->getSpace()->dispatchEvent(
        "mousedown",
        args,
        this->m_frame->getWindowElement()
    );
}

void SpaceWidget::OnKeyDown(wxKeyEvent& event) {
    auto *args = new space::SPKeyEventArgs(event.GetKeyCode(), event.CmdDown());
    space::SPEvent evt("keydown", args, nullptr);
    this->getSpace()->dispatchEvent(
        "keydown",
        args,
        this->m_frame->getWindowElement()
    );
    this->getSpace()->processEvent(evt);
    event.Skip();
}

void SpaceWidget::OnRightUp(wxMouseEvent& event)
{
    auto *args = new space::SPMouseEventArgs(1, event.GetX(), event.GetY());
    space::SPEvent evt("mouseup", args, nullptr);
    this->getSpace()->dispatchEvent(
        "mouseup",
        args,
        this->m_frame->getWindowElement()
    );
}

void SpaceWidget::OnMouseMove(wxMouseEvent& event)
{
    this->m_mouseX = event.GetX();
    this->m_mouseY = event.GetY();
    auto *args = new space::SPMouseEventArgs(0, event.GetX(), event.GetY());
    space::SPEvent evt("mousemove", args, nullptr);
    this->getSpace()->dispatchEvent(
        "mousemove",
        args,
        this->m_frame->getWindowElement()
    );
}

void SpaceWidget::Init()
{
    auto *frame = dynamic_cast<SpaceFrame *>(this->GetParent());
    this->m_frame = frame;
}

wxSize SpaceWidget::DoGetBestSize()
{
    // we need to calculate and return the best size of the widget...
    return wxSize(100, 100);
}

void SpaceWidget::OnPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);
    int width = this->GetSize().GetWidth();
    int height = this->GetSize().GetHeight();
    // draw the widget on a wxDC...
    space::platform::wxwidgets::WxWidgetsGraphicsContext* gc = dynamic_cast<space::platform::wxwidgets::WxWidgetsGraphicsContext *>(this->m_frame->getWindowElement()->createGraphics());
    gc->setFrame(this->m_frame);
    gc->setWindow(this->GetParent());
    gc->setPaintDC(&dc);
	GetClientSize( &width, &height );
    this->m_frame->getWindowElement()->draw(gc, width, height);
}
