#pragma once

#include "SPEventArgs.h"
#include <string>

namespace space {

/// Carries current playback state: playing flag, position, and duration.
/// Dispatched on SpacifyApp's playback observable whenever state changes.
class SPPlaybackStateEventArgs : public SPEventArgs {
public:
    bool playing    = false;
    int  positionMs = 0;   ///< Current playback position in milliseconds.
    int  durationMs = 0;   ///< Track duration in milliseconds (0 if unknown).

    SPPlaybackStateEventArgs(bool playing, int positionMs, int durationMs)
        : SPEventArgs(), playing(playing)
        , positionMs(positionMs), durationMs(durationMs) {}

    std::string getType() override { return "SPPlaybackStateEventArgs"; }
};

/// Carries the seek target emitted by SPSeekBarElement on mouse-click.
class SPSeekEventArgs : public SPEventArgs {
    int m_positionMs;
public:
    explicit SPSeekEventArgs(int posMs) : SPEventArgs(), m_positionMs(posMs) {}
    int getPositionMs() const { return m_positionMs; }
    std::string getType() override { return "SPSeekEventArgs"; }
};

/// Carries a table row index, dispatched as "rowdblclicked" by SPTableElement.
class SPTableRowEventArgs : public SPEventArgs {
    int m_row;
public:
    explicit SPTableRowEventArgs(int row) : SPEventArgs(), m_row(row) {}
    int getRow() const { return m_row; }
    std::string getType() override { return "SPTableRowEventArgs"; }
};

} // namespace space
