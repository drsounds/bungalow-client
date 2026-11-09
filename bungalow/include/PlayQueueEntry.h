#pragma once

#include <cstdint>
#include <string>

/// Display snapshot of one play-queue slot, decoupled from SPPlayable.
/// Built by data sources at the moment of queue construction.
struct PlayQueueEntry {
    std::string displayTitle;    // track name (e.g. "Bohemian Rhapsody")
    std::string displayArtist;   // primary artist name (e.g. "Queen")
    std::string coverArtPath;    // optional cover art (empty for bulk-built queues)
    std::string uri;             // direct playback URI, e.g. the local file (may be empty)
    bool        playable = true; // false → greyed out, skipped by next/prev
    // Identity the PlaybackCoordinator resolves across music services:
    std::string isrc;            // empty when unknown
    int64_t     trackId = 0;     // library track, 0 when not from the library
};
