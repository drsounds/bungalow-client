#pragma once

#include "SPModel.h"
#include <sqlite3.h>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class SPDBConnection;
class SPTrack;

/// Maps the `saves` table
/// (id INTEGER PK AUTOINCREMENT, track_id FK → tracks,
///  saved_at TEXT, deleted_at TEXT — NULL means not deleted).
class SPSave : public SPModel {
public:
    int64_t     id        = 0;
    int64_t     trackId   = 0;
    std::string savedAt;
    std::string deletedAt; // empty string ↔ NULL in db (not deleted)

    SPTrack* track = nullptr; // non-owning; populated by caller

    SPSave() = default;
    explicit SPSave(int64_t trackId);

    bool isPersisted() const override { return id > 0; }
    bool save(SPDBConnection& conn) override;
    bool remove(SPDBConnection& conn) override;

    static std::optional<SPSave> find(SPDBConnection& conn, int64_t id);
    static std::vector<SPSave>   findAll(SPDBConnection& conn);
    static std::vector<SPSave>   findActive(SPDBConnection& conn); // WHERE deleted_at IS NULL

private:
    static SPSave fromStatement(sqlite3_stmt* stmt);
};
