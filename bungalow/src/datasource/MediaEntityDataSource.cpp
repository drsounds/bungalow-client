#include "datasource/MediaEntityDataSource.h"

#include "SPRichText.h"

namespace space {

std::string entityLinksMarkup(const std::vector<EntityLink>& links) {
    std::string markup;
    bool clickable = false;
    for (const auto& link : links) {
        if (!markup.empty()) markup += ", ";
        if (link.uri.empty()) {
            markup += SPRichText::escape(link.name);
        } else {
            markup += SPRichText::link(link.uri, link.name);
            clickable = true;
        }
    }
    return clickable ? markup : std::string();
}

std::string MediaEntityDataSource::getRowUri(int index) {
    // The table compares this against the current playable to highlight the
    // playing row, so rows answer with their playback URI.
    const auto media = mediaAt(index);
    return media ? media->playUri() : "";
}

int MediaEntityDataSource::getTrackIdForRow(int row) {
    const auto media = mediaAt(row);
    return media ? static_cast<int>(media->trackId()) : 0;
}

bool MediaEntityDataSource::isRowPlayable(int row) {
    const auto media = mediaAt(row);
    return media && (media->isPlayable() || playableElsewhere(*media));
}

bool MediaEntityDataSource::playableElsewhere(const MediaEntity& media) const {
    // Rows with a local file never need asking.
    const std::string isrc = media.isrc();
    if (isrc.empty() || !media.playUri().empty()) return false;
    auto resolver = m_availability.lock();
    if (!resolver) return false;
    service::PlayRequest request;
    request.trackId = media.trackId();
    request.isrc    = isrc;
    request.title   = media.name();
    request.artist  = media.artistNames();
    return resolver->availabilityFor(request).playable();
}

std::vector<PlayQueueEntry> MediaEntityDataSource::buildQueueEntries() const {
    std::vector<PlayQueueEntry> entries;
    const int count = nodeCount();
    entries.reserve(static_cast<std::size_t>(count));
    for (int row = 0; row < count; ++row) {
        const auto media = mediaAt(row);
        if (!media) continue;
        PlayQueueEntry e;
        e.displayTitle  = media->name();
        e.displayArtist = media->artistNames();
        e.artistMarkup  = entityLinksMarkup(media->artistLinks());
        e.uri           = media->playUri();
        e.playable      = (media->isPlayable() && !e.uri.empty()) || playableElsewhere(*media);
        e.isrc          = media->isrc();
        e.trackId       = media->trackId();
        entries.push_back(std::move(e));
    }
    return entries;
}

} // namespace space
