#pragma once

#include "Database.h"

#include <string>
#include <vector>

/**
 * Add/edit flows for library entities, shared by the browse lists and the
 * detail pages. Each shows a platform form dialog and persists the result.
 *
 * An empty key means "create": the identifier field is then editable and,
 * when left empty, a "local:<uuid>" key is generated. Identifiers are primary
 * keys, so they are read-only when editing.
 *
 * Credited artists are entered as comma-separated names, in credit order (the
 * first is primary). A name matches an existing artist case-insensitively;
 * otherwise a new artist with a local key is created.
 */
namespace space::views::edit {

    /** @return the saved artist's ISNI, or "" when cancelled. */
    std::string artist(Database& db, const std::string& isni = "");

    /** @return the saved recording's ISRC, or "" when cancelled. */
    std::string recording(Database& db, const std::string& isrc = "");

    /** @return the saved release's UPC, or "" when cancelled. */
    std::string release(Database& db, const std::string& upc = "");

    /**
     * Pick a recording (or create one) and add it as the next track on the
     * release. @return true when a track was added.
     */
    bool addTrackToRelease(Database& db, const std::string& upc);

    /**
     * Edit a track's recording (name, version, artists) and its release (an
     * existing one by name, a new one, or none). @return true when saved.
     */
    bool track(Database& db, int trackId);

    /**
     * Ask what kind of entry to add to a playlist (track, release or artist),
     * create it in the library, and @return its content ref, or "" when cancelled.
     */
    std::string playlistEntry(Database& db);

    /** Ask for a track's position on its release. @return true when changed. */
    bool trackNumber(Database& db, int trackId, int current);

    /** "A, B, C" for a credit list, in credit order. */
    std::string joinArtistNames(const std::vector<Database::ArtistRelationship>& credits);

    /**
     * Resolve comma-separated names to credits (numbered, first primary),
     * creating artists that don't exist yet.
     */
    std::vector<Database::ArtistRelationship> resolveArtistNames(Database& db, const std::string& csv);

} // namespace space::views::edit
