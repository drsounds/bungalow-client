#pragma once

#include "SPModel.h"
#include <sqlite3.h>
#include <optional>
#include <string>
#include <vector>

class SPDBConnection;

/// Maps the `recordings` table (isrc TEXT PK, name TEXT NOT NULL, version TEXT).
class SPRecording : public SPModel {
public:
    std::string isrc;
    std::string name;
    std::string version; // empty string ↔ NULL in db

    SPRecording() = default;
    SPRecording(std::string isrc, std::string name, std::string version = {});

    bool isPersisted() const override { return !isrc.empty(); }
    bool save(SPDBConnection& conn) override;
    bool remove(SPDBConnection& conn) override;

    static std::optional<SPRecording> find(SPDBConnection& conn, const std::string& isrc);
    static std::vector<SPRecording>   findAll(SPDBConnection& conn);

private:
    static SPRecording fromStatement(sqlite3_stmt* stmt);
};
