#pragma once

#include <Windows.h>

namespace space
{
    struct SPDragData;

    // OLE drag and drop for a Space window. The window is both the source of
    // drags started by SPDragDrop::start() and the place they are dropped:
    // while such a drag is over it, the pointer is reported back to SPDragDrop.
    namespace win32
    {
        // Makes window a drop target for drags that started in it. Initializes OLE on this thread.
        void registerDropTarget(HWND window);
        void revokeDropTarget(HWND window);

        // Runs the drag until the button is released or Escape is pressed.
        // Other applications receive the URIs as text, one per line.
        // Returns true when the drag was dropped on a target that took it.
        bool doDragDrop(const SPDragData& data);
    }
}
