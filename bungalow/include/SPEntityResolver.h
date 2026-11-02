#pragma once

#include <sqlite3.h>
#include <string>

/**
 * SPEntity — a generic resolved entity read from any user-defined table.
 *
 * Fields populated:
 *   id          — UUID4 primary key
 *   type        — table name (without user_ prefix), e.g. "artists"
 *   name        — value of a column literally named "name", or first column value
 *   imageUrl    — value of column "image_url" if it exists, otherwise ""
 *   description — value of column "description" if it exists, otherwise ""
 *   uri         — canonical short URI: bungalow:<type>:<id>
 *   valid       — true if the row was found
 */
struct SPEntity {
    std::string id;
    std::string type;
    std::string name;
    std::string imageUrl;
    std::string description;
    std::string uri;
    bool        valid = false;
};

/**
 * SPEntityResolver — lightweight, stateless resolver for bungalow entity URIs.
 *
 * Supported URI forms:
 *   bungalow:table:<tablename>:<uuid>   (long navigate form used by views)
 *   bungalow:<tablename>:<uuid>         (short stored form used in BELONGS_TO columns)
 *
 * The resolver looks up the row in user_<tablename>, reads the column list from
 * db_tables, and populates an SPEntity.
 */
class SPEntityResolver {
public:
    /**
     * Resolve a URI to an SPEntity.
     * Returns entity.valid == false if the URI is malformed or the row is not found.
     */
    static SPEntity resolve(sqlite3* db, const std::string& uri);

    /** Build the canonical short URI for a custom-table row. */
    static std::string makeUri(const std::string& tableName, const std::string& uuid);

    /**
     * Parse a URI into tableName + uuid.
     * Handles both long (bungalow:table:<name>:<uuid>) and short (bungalow:<name>:<uuid>) forms.
     * Returns false on parse failure; does NOT validate db_tables membership.
     */
    static bool parseUri(const std::string& uri,
                         std::string& tableName,
                         std::string& uuid);

private:
    static SPEntity loadFromDb(sqlite3* db,
                               const std::string& tableName,
                               const std::string& uuid);
};
