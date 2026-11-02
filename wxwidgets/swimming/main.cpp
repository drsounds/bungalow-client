// swimming: runs a swimming app (a vite-plugin-swimming bundle or an HTML
// page) in a wxWidgets window drawn with WxWidgetsGraphicsContext, or, with
// --out, renders one frame to a PNG.
#include <wx/wx.h>
#include <wx/dcbuffer.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "SPClassicSkin.h"
#include "SPMouseEventArgs.h"
#include "SPSwimmingElement.h"
#include "SPWindowElement.h"
#include "Space.h"
#include "SwimmingScheduler.h"
#include "WxWidgetsGraphicsContext.h"
#include "swimming/Cli.h"

namespace {

// Paints one Space window holding a swimming app and forwards the mouse to it.
class SwimmingPanel : public wxPanel {
public:
    SwimmingPanel(wxWindow* parent, const swimming::CliOptions& options) : wxPanel(parent) {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        m_graphics = new space::platform::wxwidgets::WxWidgetsGraphicsContext();
        m_graphics->setWindow(this);
        m_space = new space::Space();
        m_space->setGraphicsContext(m_graphics);
        m_windowElement = new space::SPWindowElement(m_space, new space::SPClassicSkin());
        // Space requires a colour scheme on the window.
        m_windowElement->setColorScheme(SP_COLOR_SCHEME_LIGHT);
        m_space->addWindow(m_windowElement);
        m_app = new space::SPSwimmingElement(m_windowElement);
        m_windowElement->appendChild(m_app);
        layout(wxSize(static_cast<int>(options.width), static_cast<int>(options.height)));

        Bind(wxEVT_PAINT, &SwimmingPanel::onPaint, this);
        Bind(wxEVT_SIZE, [this](wxSizeEvent& event) {
            layout(event.GetSize());
            Refresh(false);
        });
        Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& event) {
            CaptureMouse();
            mouseEvent("mousedown", event);
        });
        Bind(wxEVT_LEFT_UP, [this](wxMouseEvent& event) {
            if (HasCapture()) ReleaseMouse();
            mouseEvent("mouseup", event);
        });
        Bind(wxEVT_MOUSE_CAPTURE_LOST, [](wxMouseCaptureLostEvent&) {});
    }

    /// Throws std::runtime_error if the app cannot be loaded.
    void start(const swimming::CliOptions& options) {
        m_app->start(options.source, space::SPSwimmingElement::fetchWith(m_graphics), swimming::createEngineFor(options));
    }

private:
    void layout(const wxSize& size) {
        m_windowElement->setAbsoluteBounds({0, 0, size.GetWidth(), size.GetHeight()});
        m_app->setX(0);
        m_app->setY(0);
        m_app->setWidth(std::max(0, size.GetWidth()));
        m_app->setHeight(std::max(0, size.GetHeight()));
        m_app->pack();
    }

    void onPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        m_graphics->setPaintDC(&dc);
        const wxSize size = GetClientSize();
        m_windowElement->draw(m_graphics, size.GetWidth(), size.GetHeight());
        m_graphics->setPaintDC(nullptr);
    }

    void mouseEvent(const char* name, const wxMouseEvent& event) {
        m_space->dispatchEvent(name, new space::SPMouseEventArgs(0, event.GetX(), event.GetY()), m_windowElement);
    }

    space::platform::wxwidgets::WxWidgetsGraphicsContext* m_graphics;
    space::Space* m_space;
    space::SPWindowElement* m_windowElement;
    space::SPSwimmingElement* m_app;
};

swimming::CliOptions options;

class SwimmingApp : public wxApp {
public:
    bool OnInit() override {
        space::platform::wxwidgets::installSwimmingScheduler();
        auto* frame = new wxFrame(nullptr, wxID_ANY, "Swimming");
        auto* panel = new SwimmingPanel(frame, options);
        frame->SetClientSize(static_cast<int>(options.width), static_cast<int>(options.height));
        try {
            panel->start(options);
        } catch (const std::exception& error) {
            std::cerr << "cannot load app: " << error.what() << std::endl;
            frame->Destroy();
            return false;
        }
        frame->Show();
        return true;
    }
};

} // namespace

wxIMPLEMENT_APP_NO_MAIN(SwimmingApp);

int main(int argc, char** argv) {
    const auto parsed = swimming::parseCli(std::vector<std::string>(argv + 1, argv + argc));
    if (!parsed) {
        std::cerr << swimming::kCliUsage << std::endl;
        return 2;
    }
    options = *parsed;
    if (options.out) {
        try {
            swimming::renderFrame(options);
        } catch (const std::exception& error) {
            std::cerr << "cannot render app: " << error.what() << std::endl;
            return 1;
        }
        return 0;
    }
    return wxEntry(argc, argv);
}
