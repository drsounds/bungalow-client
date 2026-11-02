#include "SPSwimmingElement.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

#include "SPColor.h"
#include "SPFontStyle.h"
#include "SPGraphicsContext.h"
#include "SPMouseEventArgs.h"

namespace space {
    namespace {
        /// How often JS timers run and the app is repainted while it is shown.
        constexpr int kFrameIntervalMs = 33;
        /// Apps not painted for this long are hidden and stop ticking.
        constexpr auto kShownTimeout = std::chrono::milliseconds(250);

        std::vector<SPSwimmingElement*>& liveElements() {
            static std::vector<SPSwimmingElement*> elements;
            return elements;
        }

        bool s_tickPending = false;

        SPColor toColor(const swimming::Rgba& c, float opacity) {
            return SPColor(c[0], c[1], c[2], static_cast<short>(std::lround(c[3] * opacity)));
        }

        void paintBox(const swimming::LayoutBox& b, float ox, float oy, const std::string& family, SPGraphicsContext* c) {
            const float fx = ox + b.x, fy = oy + b.y;
            const int x = static_cast<int>(std::lround(fx)), y = static_cast<int>(std::lround(fy));
            const int w = static_cast<int>(std::lround(b.w)), h = static_cast<int>(std::lround(b.h));
            const swimming::Computed& s = b.computed;
            if (b.id && w > 0 && h > 0) {
                const int radius = static_cast<int>(std::lround(std::min({s.radius, b.w / 2, b.h / 2})));
                if (s.background)
                    c->fillRoundedRectangle(x, y, w, h, radius, toColor(*s.background, s.opacity));
                // Borders are drawn inside the box, one 1px ring at a time.
                const int border = static_cast<int>(std::lround(s.borderWidth));
                for (int i = 0; i < border && 2 * i < std::min(w, h); ++i)
                    c->drawRoundedRectangle(x + i, y + i, w - 2 * i, h - 2 * i, std::max(0, radius - i),
                                            toColor(s.borderColor, s.opacity));
            }
            // Text keeps its spaces, which separate it from the text boxes beside it.
            if (b.text && b.text->find_first_not_of(" \t\r\n") != std::string::npos) {
                const SPFontStyle font(family, static_cast<int>(std::lround(s.fontSize)), s.bold ? 700 : 400, false, false);
                c->drawString(*b.text, font, toColor(s.color, s.opacity), x, y, w, h);
            }
            for (const swimming::LayoutBox& child : b.children)
                paintBox(child, fx, fy, family, c);
        }
    }

    std::function<void(int delayMs)> SPSwimmingElement::s_scheduleTick;

    SPSwimmingElement::SPSwimmingElement(SPElement* parent) : SPElement(parent) {
        setTagName("swimming");
        addEventListener("mousedown", reinterpret_cast<SPCallback>(&SPSwimmingElement::onMouseDown), this, this);
        addEventListener("mouseup", reinterpret_cast<SPCallback>(&SPSwimmingElement::onMouseUp), this, this);
        liveElements().push_back(this);
    }

    SPSwimmingElement::~SPSwimmingElement() {
        auto& elements = liveElements();
        elements.erase(std::remove(elements.begin(), elements.end(), this), elements.end());
    }

    void SPSwimmingElement::start(const std::string& source, const swimming::Fetch& fetch,
                                  std::unique_ptr<swimming::JsEngine> engine) {
        m_bridge.reset();
        if (!engine) engine = swimming::createDefaultEngine();
        // Until the first paint gives the real size.
        const sp_rectangle bounds = getAbsoluteBounds();
        auto bridge = std::make_unique<swimming::Bridge>(static_cast<float>(bounds.width > 0 ? bounds.width : 800),
                                                         static_cast<float>(bounds.height > 0 ? bounds.height : 600),
                                                         std::move(engine));
        swimming::runSource(*bridge, source, fetch);
        m_bridge = std::move(bridge);
        m_lastTick = Clock::now();
        invalidate();
    }

    void SPSwimmingElement::stop() {
        m_bridge.reset();
        invalidate();
    }

    int SPSwimmingElement::onMouseDown(SPObject*, SPEventArgs* args) {
        auto* mouse = dynamic_cast<SPMouseEventArgs*>(args);
        m_pressed = mouse && mouse->getButton() == 0;
        return m_pressed ? 1 : 0;
    }

    // A click is a left button press and release on the app; the release
    // rather than the platform's "click" event, whose button numbers differ
    // between backends.
    int SPSwimmingElement::onMouseUp(SPObject*, SPEventArgs* args) {
        auto* mouse = dynamic_cast<SPMouseEventArgs*>(args);
        if (!mouse || mouse->getButton() != 0 || !m_pressed) return 0;
        m_pressed = false;
        if (!m_bridge) return 1;
        const sp_rectangle bounds = getAbsoluteBounds();
        try {
            m_bridge->pointer("click", static_cast<float>(mouse->getX() - bounds.x), static_cast<float>(mouse->getY() - bounds.y));
        } catch (const std::exception& error) {
            std::cerr << "swimming: click handler failed: " << error.what() << std::endl;
        }
        invalidate();
        return 1;
    }

    void SPSwimmingElement::paint(int x, int y, int width, int height, SPGraphicsContext* c) {
        m_lastPaint = Clock::now();
        if (width <= 0 || height <= 0) return;
        c->fillRectangle(x, y, width, height, SPColor(255, 255, 255, 255));
        if (!m_bridge) return;
        swimming::Engine& engine = m_bridge->engine();
        if (engine.width() != static_cast<float>(width) || engine.height() != static_cast<float>(height))
            m_bridge->resize(static_cast<float>(width), static_cast<float>(height));
        if (const swimming::LaidOut* layout = engine.layout())
            paintBox(layout->root, static_cast<float>(x), static_cast<float>(y), getStringAttribute("font", "Tahoma"), c);
        if (!s_tickPending) scheduleTick(kFrameIntervalMs);
    }

    bool SPSwimmingElement::isShown() const {
        return m_bridge && isVisible() && Clock::now() - m_lastPaint < kShownTimeout;
    }

    void SPSwimmingElement::tick() {
        const auto now = Clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastTick).count();
        m_lastTick = now;
        try {
            m_bridge->tick(static_cast<std::uint32_t>(std::clamp<long long>(elapsed, 0, 0xFFFFFFFFLL)));
        } catch (const std::exception& error) {
            std::cerr << "swimming: timer failed: " << error.what() << std::endl;
        }
        invalidate();
    }

    void SPSwimmingElement::scheduleTick(int delayMs) {
        if (!s_scheduleTick) return;
        s_tickPending = true;
        s_scheduleTick(delayMs);
    }

    swimming::Fetch SPSwimmingElement::fetchWith(SPGraphicsContext* c) {
        struct Response {
            bool answered = false;
            bool ok = false;
            std::string body;
        };
        return [c](const std::string& url) {
            // Shared, as an asynchronous answer arrives after this returns.
            auto response = std::make_shared<Response>();
            c->request("GET", url, "", [response](bool success, const std::string& body) {
                response->answered = true;
                response->ok = success;
                response->body = body;
            });
            if (!response->answered) throw std::runtime_error("cannot load " + url + ": this backend fetches asynchronously");
            if (!response->ok) throw std::runtime_error("cannot load " + url);
            return response->body;
        };
    }

    void SPSwimmingElement::tickAll() {
        s_tickPending = false;
        const auto frame = std::chrono::milliseconds(kFrameIntervalMs);
        int next = -1;
        const std::vector<SPSwimmingElement*> elements = liveElements(); // ticks may create or destroy apps
        for (SPSwimmingElement* element : elements) {
            if (std::find(liveElements().begin(), liveElements().end(), element) == liveElements().end()) continue;
            if (!element->isShown()) continue;
            const auto since = Clock::now() - element->m_lastTick;
            int wait = kFrameIntervalMs;
            if (since >= frame)
                element->tick();
            else
                wait = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(frame - since).count());
            next = next < 0 ? wait : std::min(next, wait);
        }
        if (next >= 0) scheduleTick(std::max(1, next));
    }
}
