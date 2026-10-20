#pragma once

#include <string>

/// Display snapshot of one play-queue slot, decoupled from SPPlayable.
/// Built by data sources at the moment of queue construction.
struct PlayQueueEntry {
    std::string displayTitle;    // track name (e.g. "Bohemian Rhapsody")
    std::string displayArtist;   // primary artist name (e.g. "Queen")
    std::string coverArtPath;    // optional cover art (empty for bulk-built queues)
    std::string uri;             // resolved playback URI (empty when !playable)
    bool        playable = true; // false → greyed out, skipped by next/prev
};
