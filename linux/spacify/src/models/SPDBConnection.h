#pragma once

#include "../Database.h"
#include <sqlite3.h>
#include <stdexcept>
#include <string>

/// Thin adapter between SPModel subclasses and the underlying Database.
/// Provides statement preparation and execution helpers so models never
/// touch the raw sqlite3* handle directly.
class SPDBConnection {
public:
    explicit SPDBConnection(Database& db) : m_db(db) {}

    sqlite3* handle() const noexcept { return m_db.handle(); }

    /// Prepare a SQL statement; throws std::runtime_error on failure.
    sqlite3_stmt* prepare(const char* sql) const {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(m_db.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK)
            throw std::runtime_error(std::string("SPDBConnection::prepare failed: ") +
                                     sqlite3_errmsg(m_db.handle()));
        return stmt;
    }

    /// Execute a one-shot statement with no result rows; throws on failure.
    void exec(const char* sql) const {
        char* err = nullptr;
        if (sqlite3_exec(m_db.handle(), sql, nullptr, nullptr, &err) != SQLITE_OK) {
            std::string msg(err);
            sqlite3_free(err);
            throw std::runtime_error("SPDBConnection::exec failed: " + msg);
        }
    }

private:
    Database& m_db;
};