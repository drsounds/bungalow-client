#ifndef SWIMMING_BRIDGE_H
#define SWIMMING_BRIDGE_H

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "swimming/Engine.h"
#include "swimming/JsEngine.h"
#include "swimming/Render.h"

/// Embeds a JavaScript engine, exposes the native __SWIMMING_NATIVE__
/// command API to the React reconciler and routes native events back into JS.
namespace swimming {
    class Bridge {
    public:
        /// Throws JsError if the engine cannot start.
        Bridge(float width, float height, std::unique_ptr<JsEngine> js = createDefaultEngine());
        Bridge(const Bridge&) = delete;
        Bridge& operator=(const Bridge&) = delete;

        Engine& engine() { return m_engine; }
        const Engine& engine() const { return m_engine; }

        /// Name of the underlying JavaScript engine.
        const char* jsEngineName() const { return m_js->name(); }

        void resize(float width, float height);

        /// Runs a script (e.g. the bundle from vite-plugin-swimming), then
        /// its promise jobs and due timers. Throws JsError.
        void eval(const std::string& source, const std::string& fileName = "<script>");

        /// The engines have no event loop: drains promise jobs and runs
        /// polyfilled timers (__SWIMMING_TICK__) until idle.
        void runUntilIdle();

        /// Advances the timer clock by elapsedMs and runs what is due.
        void tick(std::uint32_t elapsedMs);

        /// Hit-tests natively, then calls __SWIMMING_DISPATCH__(id, event) so
        /// JS handlers run. Returns the element hit, if any.
        std::optional<std::uint64_t> pointer(const std::string& event, float x, float y);

        /// Replaces the polyfilled history entry with path ("/a/b") and fires
        /// popstate, so routers that read window.location follow it.
        void replaceRoute(const std::string& path);

        /// Paints the last layout without glyphs (see Render.h).
        Pixmap render() const;

    private:
        void registerGlobals();

        // Declared before m_js, so the natives JS holds outlive it.
        Engine m_engine;
        std::unique_ptr<JsEngine> m_js;
    };
}

#endif
