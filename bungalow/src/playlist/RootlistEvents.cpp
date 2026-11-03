#include "playlist/RootlistEvents.h"

#include <utility>
#include <vector>

namespace space {
namespace playlist {
namespace {

std::vector<std::pair<int, RootlistEvents::Listener>>& listeners() {
    static std::vector<std::pair<int, RootlistEvents::Listener>> list;
    return list;
}

} // namespace

int RootlistEvents::addListener(Listener listener) {
    static int nextId = 0;
    listeners().emplace_back(++nextId, std::move(listener));
    return nextId;
}

void RootlistEvents::removeListener(int id) {
    auto& list = listeners();
    for (auto it = list.begin(); it != list.end(); ++it) {
        if (it->first == id) {
            list.erase(it);
            return;
        }
    }
}

void RootlistEvents::fireChanged() {
    // A listener may add or remove listeners while being notified.
    const auto snapshot = listeners();
    for (const auto& [id, listener] : snapshot) {
        if (listener) listener();
    }
}

} // namespace playlist
} // namespace space
