#ifndef SP_SWIMMING_ELEMENT_H
#define SP_SWIMMING_ELEMENT_H

#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "SPElement.h"
#include "swimming/Loader.h"

namespace space {
    /// Runs a swimming app (a React app built with vite-plugin-swimming, or
    /// an HTML page with classic scripts) and draws its layout with the
    /// graphics context: backgrounds, borders, rounded corners and text.
    ///
    /// Left clicks reach the app's handlers. While the element is being
    /// painted, its JS timers run about 30 times a second through
    /// s_scheduleTick, which the platform layer provides; apps not painted
    /// for a while stop ticking.
    class SPSwimmingElement : public SPElement {
    public:
        explicit SPSwimmingElement(SPElement* parent);
        ~SPSwimmingElement() override;

        string getType() override { return "swimming"; }

        /// Starts source (see swimming::runSource) in a new JS runtime,
        /// replacing the running app. Throws std::runtime_error (including
        /// swimming::JsError) if the runtime or the app fails to start,
        /// leaving no app running.
        void start(const std::string& source, const swimming::Fetch& fetch = {});
        /// Stops the running app, if any.
        void stop();
        /// The running app's bridge, or null.
        swimming::Bridge* getBridge() const { return m_bridge.get(); }

        void paint(int x, int y, int width, int height, SPGraphicsContext* c) override;

        /// Set by the platform layer: calls tickAll() once, on the UI thread,
        /// after delayMs.
        static std::function<void(int delayMs)> s_scheduleTick;

        /// Runs the JS timers of the apps being shown and schedules the next
        /// frame while any is shown.
        static void tickAll();

        /// A fetch over c->request(), for http(s) sources. Fails on graphics
        /// contexts that answer requests asynchronously.
        static swimming::Fetch fetchWith(SPGraphicsContext* c);

    private:
        using Clock = std::chrono::steady_clock;

        int onMouseDown(SPObject* sender, SPEventArgs* args);
        int onMouseUp(SPObject* sender, SPEventArgs* args);
        bool isShown() const;
        void tick();
        static void scheduleTick(int delayMs);

        std::unique_ptr<swimming::Bridge> m_bridge;
        bool m_pressed = false; // a left button press started on the app
        Clock::time_point m_lastPaint{};
        Clock::time_point m_lastTick = Clock::now();
    };
}

#endif
