#pragma once

#include <string>

/// A service account. Plain value type, not DB-mapped.
struct SPUser {
    std::string id;
    std::string name;

    bool isValid() const { return !id.empty(); }
};
