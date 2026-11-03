#ifndef SP_SCROLLBAR_H
#define SP_SCROLLBAR_H

#include "libspace.h"

namespace space {
    /// Geometry of a vertical scrollbar, provided by the skin.
    ///
    /// The up and down buttons may overflow into the track by upOverflow and
    /// downOverflow pixels (the bottom of the up button and the top of the
    /// down button, e.g. a concave cap). The track runs under that overflow,
    /// and the thumb travels all the way to its edges.
    struct SPScrollBarMetrics {
        int width = 10;
        int upHeight = 0;
        int downHeight = 0;
        int upOverflow = 0;
        int downOverflow = 0;
        int minThumbHeight = 20;
    };

    enum class SPScrollBarPart { Up, Down, Track, Thumb };

    /// Where the parts of a vertical scrollbar go, in window coordinates.
    struct SPScrollBarLayout {
        sp_rectangle up { 0, 0, 0, 0, 0 };
        sp_rectangle down { 0, 0, 0, 0, 0 };
        sp_rectangle track { 0, 0, 0, 0, 0 };
        sp_rectangle thumb { 0, 0, 0, 0, 0 };
        /// False when the track is too short for a thumb.
        bool hasThumb = false;
    };
}

#endif // SP_SCROLLBAR_H
