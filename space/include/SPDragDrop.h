#ifndef SPDRAGDROP_H
#define SPDRAGDROP_H

#include <string>
#include <vector>

namespace space {
class SPElement;

/** What a drag carries: the URIs of the dragged entities. */
struct SPDragData {
    std::vector<std::string> uris;
};

/**
 * SPDropTarget — something a drag can be dropped on. Register it for an
 * element with SPDragDrop::registerTarget(); it is then asked about drags
 * whose pointer is inside that element. Points are in window coordinates.
 */
class SPDropTarget {
public:
    virtual ~SPDropTarget() = default;

    /**
     * The drag is over the target. Called on every pointer move and
     * repeatedly while the pointer rests, so targets can act on a hover.
     * @return whether a drop at this point would be taken.
     */
    virtual bool dragOver(int x, int y, const SPDragData& data) = 0;
    /** The drag left the target, or ended elsewhere. */
    virtual void dragLeave() {}
    /** The drag was released over the target. @return whether it was taken. */
    virtual bool drop(int x, int y, const SPDragData& data) = 0;
};

/**
 * SPDragDrop — the drag in progress, and the drop targets of the window.
 *
 * An element starts a drag with start(), which hands it to the OS layer
 * through SPGraphicsContext::startDrag(). While the OS drags, its layer
 * reports the pointer back through over() / leave() / drop(), which find the
 * target under the pointer.
 */
class SPDragDrop {
public:
    static void registerTarget(SPElement* element, SPDropTarget* target);
    static void unregisterTarget(SPDropTarget* target);

    /**
     * Drag @p data out of @p source. Returns when the drag has ended;
     * true when it was dropped on a target that took it.
     */
    static bool start(SPElement* source, SPDragData data);

    // ── For the OS layers ─────────────────────────────────────────────────────

    /** Whether a drag started by start() is in progress. */
    static bool active();
    static const SPDragData& data();
    /** The pointer is at (x, y). @return whether a drop there would be taken. */
    static bool over(int x, int y);
    /** The pointer left the window. */
    static void leave();
    /** The drag was released at (x, y). @return whether it was taken. */
    static bool drop(int x, int y);
};

} // namespace space
#endif // SPDRAGDROP_H
