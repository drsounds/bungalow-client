#include "app/library/MusicEntityDataSource.h"

namespace space {

MusicEntityDataSource::MusicEntityDataSource(std::vector<MusicColumn> columns)
    : m_columns(std::move(columns)) {}

MusicEntityDataSource::Row MusicEntityDataSource::rowAt(int index) const {
    if (index < 0 || index >= static_cast<int>(m_rows.size())) return nullptr;
    return m_rows[index];
}

std::string MusicEntityDataSource::getTextForColumn(int column) {
    if (column < 0 || column >= static_cast<int>(m_columns.size())) return "";
    return musicColumnTitle(m_columns[column]);
}

std::string MusicEntityDataSource::getTextForCell(int column, int row) {
    const Row entity = rowAt(row);
    if (!entity || column < 0 || column >= static_cast<int>(m_columns.size())) return "";
    return entity->text(m_columns[column]);
}

std::string MusicEntityDataSource::getMarkupForCell(int column, int row) {
    const Row entity = rowAt(row);
    if (!entity || column < 0 || column >= static_cast<int>(m_columns.size())) return "";
    return entityLinksMarkup(entity->links(m_columns[column]));
}

} // namespace space
