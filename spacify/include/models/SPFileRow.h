#pragma once

#include "SPModel.h"
#include <sqlite3.h>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class SPDBConnection;

/// Maps the `files` table (id INTEGER PK AUTOINCREMENT, path TEXT NOT NULL UNIQUE).
class SPFileRow : public SPModel {
public:
    int64_t     id = 0;
    std::string path;

    SPFileRow() = default;
    explicit SPFileRow(std::string path);
    SPFileRow(int64_t id, std::string path);

    bool isPersisted() const override { return id > 0; }
    bool save(SPDBConnection& conn) override;
    bool remove(SPDBConnection& conn) override;

    static std::optional<SPFileRow> find(SPDBConnection& conn, int64_t id);
    static std::optional<SPFileRow> findByPath(SPDBConnection& conn, const std::string& path);
    static std::vector<SPFileRow>   findAll(SPDBConnection& conn);

private:
    static SPFileRow fromStatement(sqlite3_stmt* stmt);
};
