#pragma once

#include "PlayQueueEntry.h"

#include <cstddef>
#include <vector>

/// Passed by a view to BungalowApp::playContext() to start a full play action.
/// Contains the entire list of entries (in display order) and which one to start at.
struct PlayContext {
    std::vector<PlayQueueEntry> entries;
    std::size_t                 startIndex = 0;
    /// "Play with…": ask which service plays the start entry, even when only
    /// one can or a choice was remembered.
    bool                        chooseService = false;
};
