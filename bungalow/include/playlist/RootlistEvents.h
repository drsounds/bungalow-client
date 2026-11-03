#pragma once

#include <functional>

namespace space {
namespace playlist {

/// Change notifications for the rootlist. RootlistManager fires them after
/// every change; the playlist concept listens to keep the sidebar current.
/// UI thread only.
class RootlistEvents {
public:
    using Listener = std::function<void()>;

    RootlistEvents() = delete;

    /// Returns an id for removeListener().
    static int addListener(Listener listener);
    static void removeListener(int id);
    static void fireChanged();
};

} // namespace playlist
} // namespace space
