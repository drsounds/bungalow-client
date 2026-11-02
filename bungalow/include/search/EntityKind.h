#pragma once

namespace space {
namespace search {

/// The domain abstraction a SearchResult represents. Decides the section a
/// result is grouped under in the search view, and the sections' order.
enum class EntityKind {
    Artist,
    Release,
    Recording,
    Track,
    Video,
    Other,
};

/// Section heading shown in the search view.
inline const char* label(EntityKind kind) {
    switch (kind) {
        case EntityKind::Artist:    return "Artists";
        case EntityKind::Release:   return "Releases";
        case EntityKind::Recording: return "Songs";
        case EntityKind::Track:     return "Tracks";
        case EntityKind::Video:     return "Videos";
        case EntityKind::Other:     break;
    }
    return "Other";
}

/// Sort key for ordering sections.
inline int order(EntityKind kind) {
    switch (kind) {
        case EntityKind::Artist:    return 1;
        case EntityKind::Release:   return 2;
        case EntityKind::Recording: return 3;
        case EntityKind::Track:     return 4;
        case EntityKind::Video:     return 5;
        case EntityKind::Other:     break;
    }
    return 99;
}

} // namespace search
} // namespace space
