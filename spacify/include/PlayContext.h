#pragma once

#include "PlayQueueEntry.h"

#include <cstddef>
#include <vector>

/// Passed by a view to SpacifyApp::playContext() to start a full play action.
/// Contains the entire list of entries (in display order) and which one to start at.
struct PlayContext {
    std::vector<PlayQueueEntry> entries;
    std::size_t                 startIndex = 0;
};
