#pragma once

#include "SPModel.h"
#include <sqlite3.h>
#include <optional>
#include <string>
#include <vector>

class SPDBConnection;

/// Maps the `artists` table (isni TEXT PK, name TEXT NOT NULL).
class SPArtist : public SPModel {
public:
    std::string isni;
    std::string name;

    SPArtist() = default;
    SPArtist(std::string isni, std::string name);

    bool isPersisted() const override { return !isni.empty(); }
    bool save(SPDBConnection& conn) override;
    bool remove(SPDBConnection& conn) override;

    static std::optional<SPArtist> find(SPDBConnection& conn, const std::string& isni);
    static std::vector<SPArtist>   findAll(SPDBConnection& conn);

private:
    static SPArtist fromStatement(sqlite3_stmt* stmt);
};
