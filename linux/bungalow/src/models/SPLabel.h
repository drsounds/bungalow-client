#pragma once

#include "SPModel.h"
#include <sqlite3.h>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class SPDBConnection;

/// Maps the `labels` table (id INTEGER PK AUTOINCREMENT, name TEXT NOT NULL).
class SPLabel : public SPModel {
public:
    int64_t     id = 0;
    std::string name;

    SPLabel() = default;
    explicit SPLabel(std::string name);
    SPLabel(int64_t id, std::string name);

    bool isPersisted() const override { return id > 0; }
    bool save(SPDBConnection& conn) override;
    bool remove(SPDBConnection& conn) override;

    static std::optional<SPLabel> find(SPDBConnection& conn, int64_t id);
    static std::vector<SPLabel>   findAll(SPDBConnection& conn);

private:
    static SPLabel fromStatement(sqlite3_stmt* stmt);
};