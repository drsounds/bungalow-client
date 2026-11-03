#include "MusicEntityDataSource.h"

namespace space {

MusicEntityDataSource::MusicEntityDataSource(std::vector<std::string> columns)
    : m_columns(std::move(columns)) {}

const MusicEntityRow* MusicEntityDataSource::rowAt(int index) const {
    if (index < 0 || index >= static_cast<int>(m_rows.size())) return nullptr;
    return &m_rows[index];
}

std::string MusicEntityDataSource::getTextForColumn(int column) {
    if (column < 0 || column >= static_cast<int>(m_columns.size())) return "";
    return m_columns[column];
}

std::string MusicEntityDataSource::getTextForCell(int column, int row) {
    const MusicEntityRow* r = rowAt(row);
    if (!r || column < 0 || column >= static_cast<int>(r->cells.size())) return "";
    return r->cells[column];
}

std::string MusicEntityDataSource::getRowUri(int index) {
    // The table compares this against the current playable to highlight the
    // playing row, so tracks answer with their playback URI.
    const MusicEntityRow* r = rowAt(index);
    return r ? r->playUri : "";
}

int MusicEntityDataSource::getTrackIdForRow(int row) {
    const MusicEntityRow* r = rowAt(row);
    return r ? r->trackId : 0;
}

bool MusicEntityDataSource::isRowPlayable(int row) {
    const MusicEntityRow* r = rowAt(row);
    return r && r->playable;
}

std::vector<PlayQueueEntry> MusicEntityDataSource::buildQueueEntries() const {
    std::vector<PlayQueueEntry> entries;
    entries.reserve(m_rows.size());
    for (const auto& r : m_rows) {
        PlayQueueEntry e;
        e.displayTitle  = r.title;
        e.displayArtist = r.artist;
        e.uri           = r.playUri;
        e.playable      = r.playable && !r.playUri.empty();
        entries.push_back(std::move(e));
    }
    return entries;
}

} // namespace space
