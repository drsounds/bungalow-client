#pragma once

#include <string>
#include <chrono>
#include <functional>
#include <utility>
#include <vector>

namespace space {
class SPTableElement;

namespace views {

class MusicDragDrop {
public:
    static void begin(std::vector<std::string> refs, SPTableElement* source) {
        State& value = state();
        value.refs = std::move(refs);
        value.source = source;
        value.active = !value.refs.empty();
        value.released = false;
    }

    static bool active() { return state().active; }
    static bool released() { return state().released; }
    static SPTableElement* source() { return state().source; }
    static const std::vector<std::string>& refs() { return state().refs; }
    static const std::string& hoverUri() { return state().hoverUri; }
    static void markReleased() { state().released = true; }
    static void setHoverCallback(std::function<void()> callback) {
        state().onHoverDwell = std::move(callback);
    }
    // The dragging table holds the mouse capture, so other panes (the sidebar)
    // never get mouse events; the table forwards them through these instead.
    using PointerCallback = std::function<void(int x, int y)>;
    static void setPointerCallbacks(PointerCallback move, PointerCallback end) {
        state().onMove = std::move(move);
        state().onEnd = std::move(end);
    }
    static void dispatchMove(int x, int y) {
        const auto callback = state().onMove;
        if (callback) callback(x, y);
    }
    static void dispatchEnd(int x, int y) {
        const auto callback = state().onEnd;
        if (callback) callback(x, y);
    }
    static void setHoverTarget(const std::string& uri) {
        State& value = state();
        if (value.hoverUri == uri) return;
        value.hoverUri = uri;
        value.hoverStarted = std::chrono::steady_clock::now();
        value.hoverNavigated = false;
    }
    static void clearHoverTarget() { setHoverTarget({}); }
    static void checkHoverDwell() {
        State& value = state();
        if (!value.active || value.released || value.hoverUri.empty() || value.hoverNavigated ||
            std::chrono::steady_clock::now() - value.hoverStarted < std::chrono::milliseconds(700))
            return;
        value.hoverNavigated = true;
        if (value.onHoverDwell) {
            const auto callback = value.onHoverDwell;
            callback();
        }
    }
    static void clear() {
        State& value = state();
        value.refs.clear();
        value.source = nullptr;
        value.active = false;
        value.released = false;
        clearHoverTarget();
    }

private:
    struct State {
        std::vector<std::string> refs;
        SPTableElement* source = nullptr;
        bool active = false;
        bool released = false;
        std::string hoverUri;
        std::chrono::steady_clock::time_point hoverStarted;
        bool hoverNavigated = false;
        std::function<void()> onHoverDwell;
        PointerCallback onMove;
        PointerCallback onEnd;
    };

    static State& state() {
        static State value;
        return value;
    }
};

} // namespace views
} // namespace space
