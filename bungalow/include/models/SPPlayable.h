#pragma once

#include "SPModel.h"
#include <sqlite3.h>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class SPDBConnection;
class SPMediaService;

/// Maps the `playables` table (id INTEGER PK AUTOINCREMENT, uri TEXT NOT NULL UNIQUE).
class SPPlayable : public SPModel {
public:
    int64_t     id = 0;
    std::string uri;

    /// The media service responsible for playing this URI.
    /// Resolved from the registered service list via resolveService().
    /// Non-owning – lifetime is managed by BungalowApp.
    SPMediaService* service = nullptr;

    SPPlayable() = default;
    explicit SPPlayable(std::string uri);
    SPPlayable(int64_t id, std::string uri);

    /// Walk @p services and set service to the first entry that acceptsUri(uri).
    void resolveService(const std::vector<SPMediaService*>& services);

    bool isPersisted() const override { return id > 0; }
    bool save(SPDBConnection& conn) override;
    bool remove(SPDBConnection& conn) override;

    static std::optional<SPPlayable> find(SPDBConnection& conn, int64_t id);
    static std::optional<SPPlayable> findByUri(SPDBConnection& conn, const std::string& uri);
    static std::vector<SPPlayable>   findAll(SPDBConnection& conn);

private:
    static SPPlayable fromStatement(sqlite3_stmt* stmt);
};
