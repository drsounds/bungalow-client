#include "SPDragDrop.h"

#include "SPElement.h"
#include "SPGraphicsContext.h"
#include "Space.h"

#include <algorithm>
#include <utility>

namespace space {
namespace {

struct Registration {
    SPElement*    element;
    SPDropTarget* target;
};

struct State {
    std::vector<Registration> targets;
    SPDropTarget*             hovered = nullptr; // the target the drag is over
    SPDragData                data;
    bool                      active = false;
};

State& state() {
    static State value;
    return value;
}

// The target whose element is shown and contains the point; the one
// registered last wins where elements overlap.
SPDropTarget* targetAt(int x, int y) {
    const auto& targets = state().targets;
    for (auto it = targets.rbegin(); it != targets.rend(); ++it) {
        bool visible = true;
        for (SPElement* element = it->element; element; element = element->getParentElement())
            if (!element->isVisible()) { visible = false; break; }
        if (!visible) continue;
        const auto bounds = it->element->getAbsoluteBounds();
        if (x >= bounds.x && x < bounds.x + bounds.width &&
            y >= bounds.y && y < bounds.y + bounds.height)
            return it->target;
    }
    return nullptr;
}

void setHovered(SPDropTarget* target) {
    State& value = state();
    if (value.hovered == target) return;
    SPDropTarget* previous = value.hovered;
    value.hovered = target;
    if (previous) previous->dragLeave();
}

} // namespace

void SPDragDrop::registerTarget(SPElement* element, SPDropTarget* target) {
    if (!element || !target) return;
    unregisterTarget(target);
    state().targets.push_back({element, target});
}

void SPDragDrop::unregisterTarget(SPDropTarget* target) {
    State& value = state();
    auto& targets = value.targets;
    targets.erase(std::remove_if(targets.begin(), targets.end(),
                                 [target](const Registration& r) { return r.target == target; }),
                  targets.end());
    if (value.hovered == target) value.hovered = nullptr;
}

bool SPDragDrop::start(SPElement* source, SPDragData data) {
    State& value = state();
    if (value.active || !source || data.uris.empty()) return false;
    Space* space = source->getSpace();
    SPGraphicsContext* graphics = space ? space->getGraphicsContext() : nullptr;
    if (!graphics) return false;

    value.data = std::move(data);
    value.active = true;
    const bool dropped = graphics->startDrag(value.data, source);
    setHovered(nullptr);
    value.active = false;
    value.data = {};
    return dropped;
}

bool SPDragDrop::active() {
    return state().active;
}

const SPDragData& SPDragDrop::data() {
    return state().data;
}

bool SPDragDrop::over(int x, int y) {
    if (!state().active) return false;
    SPDropTarget* target = targetAt(x, y);
    setHovered(target);
    return target && target->dragOver(x, y, state().data);
}

void SPDragDrop::leave() {
    setHovered(nullptr);
}

bool SPDragDrop::drop(int x, int y) {
    State& value = state();
    if (!value.active) return false;
    SPDropTarget* target = targetAt(x, y);
    // The drop ends the hover without a dragLeave() for the target that gets it.
    if (value.hovered != target) setHovered(nullptr);
    value.hovered = nullptr;
    return target && target->drop(x, y, value.data);
}

} // namespace space
