#pragma once

#include "SPModel.h"
#include <sqlite3.h>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class SPDBConnection;
class SPLabel;

/// Maps the `releases` table
/// (upc TEXT PK, name TEXT NOT NULL, release_date TEXT,
///  label_id INTEGER → labels, cover_art TEXT).
class SPRelease : public SPModel {
public:
    std::string upc;
    std::string name;
    std::string releaseDate; // empty ↔ NULL
    int64_t     labelId  = 0; // 0 ↔ NULL
    std::string coverArt; // empty ↔ NULL

    SPLabel* label = nullptr; // non-owning; populated by caller

    SPRelease() = default;
    SPRelease(std::string upc, std::string name,
              std::string releaseDate = {}, int64_t labelId = 0,
              std::string coverArt = {});

    bool isPersisted() const override { return !upc.empty(); }
    bool save(SPDBConnection& conn) override;
    bool remove(SPDBConnection& conn) override;

    static std::optional<SPRelease> find(SPDBConnection& conn, const std::string& upc);
    static std::vector<SPRelease>   findAll(SPDBConnection& conn);

private:
    static SPRelease fromStatement(sqlite3_stmt* stmt);
};