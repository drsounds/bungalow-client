#pragma once

#include "datasource/NodeDataSource.h"

#include <vector>

namespace space {

/**
 * EntityDataSource — a table of entities, each with a portable URI. Rows are
 * dragged as those URIs.
 */
class EntityDataSource : public NodeDataSource {
public:
    /** The row at @p row, or nullptr when out of range. */
    virtual std::shared_ptr<const Entity> entityAt(int row) const = 0;

    std::shared_ptr<const Node> nodeAt(int row) const final { return entityAt(row); }

    /** The URIs of @p rows, skipping rows that are out of range or have none. */
    std::vector<std::string> urisForRows(const std::vector<int>& rows) const;

    // ── SPTableDataSource interface ───────────────────────────────────────────
    std::vector<std::string> getDragUris(const std::vector<int>& rows) override { return urisForRows(rows); }
};

} // namespace space
