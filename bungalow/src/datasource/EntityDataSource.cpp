#include "datasource/EntityDataSource.h"

namespace space {

std::vector<std::string> EntityDataSource::urisForRows(const std::vector<int>& rows) const {
    std::vector<std::string> uris;
    for (const int row : rows) {
        const auto entity = entityAt(row);
        if (!entity) continue;
        std::string uri = entity->uri();
        if (!uri.empty()) uris.push_back(std::move(uri));
    }
    return uris;
}

} // namespace space
