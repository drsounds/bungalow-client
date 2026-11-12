#include "datasource/NodeDataSource.h"

namespace space {

std::string NodeDataSource::getTextForColumn(int column) {
    return column == 0 ? "Name" : "";
}

std::string NodeDataSource::getTextForCell(int column, int row) {
    const auto node = nodeAt(row);
    return node && column == 0 ? node->name() : "";
}

} // namespace space
