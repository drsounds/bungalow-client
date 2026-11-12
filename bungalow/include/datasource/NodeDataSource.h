#pragma once

#include "SPTableDataSource.h"
#include "datasource/Node.h"

#include <memory>
#include <string>

namespace space {

/**
 * NodeDataSource — a table of nodes. On its own it shows one "Name" column;
 * subclasses narrow the row type and add what that type knows.
 *
 * Rows are handed out as shared pointers, so a caller can keep one alive
 * while the source is refilled underneath it.
 */
class NodeDataSource : public SPTableDataSource {
public:
    ~NodeDataSource() override = default;

    virtual int nodeCount() const = 0;
    /** The row at @p row, or nullptr when out of range. */
    virtual std::shared_ptr<const Node> nodeAt(int row) const = 0;

    // ── SPTableDataSource interface ───────────────────────────────────────────
    int         getRowCount()                       override { return nodeCount(); }
    int         getColumnCount()                    override { return 1; }
    std::string getTextForColumn(int column)        override;
    std::string getTextForCell(int column, int row) override;
};

} // namespace space
