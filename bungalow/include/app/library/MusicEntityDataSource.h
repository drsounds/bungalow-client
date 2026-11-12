#pragma once

#include "app/library/MusicEntity.h"
#include "datasource/MediaEntityDataSource.h"

#include <memory>
#include <string>
#include <vector>

namespace space {

/**
 * MusicEntityDataSource
 *
 * The table behind the library and playlist views: a list of music entities
 * (artists, recordings, releases, tracks) shown through a chosen set of
 * columns. Each cell is asked of the row's entity, which answers from its
 * ORM models. Views refill the rows from a MusicEntityRepository on
 * navigate() and after every mutation.
 */
class MusicEntityDataSource : public MediaEntityDataSource {
public:
    using Row = std::shared_ptr<const MusicEntity>;

    explicit MusicEntityDataSource(std::vector<MusicColumn> columns);
    ~MusicEntityDataSource() override = default;

    /** Replace the rows. Accepts a list of any kind of music entity. */
    template <typename EntityT>
    void setEntities(const std::vector<std::shared_ptr<const EntityT>>& entities) {
        m_rows.assign(entities.begin(), entities.end());
    }

    /** The row at @p index, or nullptr when out of range. */
    Row rowAt(int index) const;

    /** The row at @p index as an @p EntityT, or nullptr when it is another kind. */
    template <typename EntityT>
    std::shared_ptr<const EntityT> rowAs(int index) const {
        return std::dynamic_pointer_cast<const EntityT>(rowAt(index));
    }

    // ── MediaEntityDataSource interface ───────────────────────────────────────
    int nodeCount() const override { return static_cast<int>(m_rows.size()); }
    std::shared_ptr<const MediaEntity> mediaAt(int row) const override { return rowAt(row); }

    // ── SPTableDataSource interface ───────────────────────────────────────────
    int         getColumnCount()                    override { return static_cast<int>(m_columns.size()); }
    std::string getTextForColumn(int column)        override;
    std::string getTextForCell(int column, int row) override;

private:
    std::vector<MusicColumn> m_columns;
    std::vector<Row>         m_rows;
};

} // namespace space
