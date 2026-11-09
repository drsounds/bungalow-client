#pragma once

#include "SPModel.h"
#include <sqlite3.h>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class SPDBConnection;

/// Maps the `playables` table (id INTEGER PK AUTOINCREMENT, uri TEXT NOT NULL UNIQUE).
/// Which service plays a URI is decided at play time by service::PlaybackCoordinator.
class SPPlayable : public SPModel {
public:
    int64_t     id = 0;
    std::string uri;

    SPPlayable() = default;
    explicit SPPlayable(std::string uri);
    SPPlayable(int64_t id, std::string uri);

    bool isPersisted() const override { return id > 0; }
    bool save(SPDBConnection& conn) override;
    bool remove(SPDBConnection& conn) override;

    static std::optional<SPPlayable> find(SPDBConnection& conn, int64_t id);
    static std::optional<SPPlayable> findByUri(SPDBConnection& conn, const std::string& uri);
    static std::vector<SPPlayable>   findAll(SPDBConnection& conn);

private:
    static SPPlayable fromStatement(sqlite3_stmt* stmt);
};
