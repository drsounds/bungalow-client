#include "PlayQueueDataSource.h"

namespace space {

int PlayQueueDataSource::getRowCount() {
    return static_cast<int>(m_entries.size());
}

std::string PlayQueueDataSource::getTextForColumn(int column) {
    return column == 0 ? "Track" : "Artist";
}

std::string PlayQueueDataSource::getTextForCell(int column, int row) {
    if (row < 0 || row >= static_cast<int>(m_entries.size())) return "";
    const auto& e = m_entries[row];
    return column == 0 ? e.displayTitle : e.displayArtist;
}

bool PlayQueueDataSource::isRowPlayable(int row) {
    if (row < 0 || row >= static_cast<int>(m_entries.size())) return false;
    return m_entries[row].playable;
}

bool PlayQueueDataSource::isRowActive(int index) {
    return index == m_activeIndex;
}

void PlayQueueDataSource::setEntries(const std::vector<PlayQueueEntry>& entries,
                                      int activeIndex) {
    m_entries     = entries;
    m_activeIndex = activeIndex;
}

} // namespace space
