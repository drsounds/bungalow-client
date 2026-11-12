#pragma once

#include <string>

class Database;
class wxWindow;

/**
 * Add/edit flows for library entities, shared by the browse lists and the
 * detail pages. Each shows a modal dialog and persists the result.
 *
 * An empty key means "create": the identifier field is then editable and,
 * when left empty, a "local:<uuid>" key is generated. Identifiers are primary
 * keys, so they are read-only when editing.
 */
namespace space::views::edit {

    /** @return the saved artist's ISNI, or "" when cancelled. */
    std::string artist(wxWindow* parent, Database& db, const std::string& isni = "");

    /** @return the saved recording's ISRC, or "" when cancelled. */
    std::string recording(wxWindow* parent, Database& db, const std::string& isrc = "");

    /** @return the saved release's UPC, or "" when cancelled. */
    std::string release(wxWindow* parent, Database& db, const std::string& upc = "");

    /**
     * Pick a recording (or create one) and add it as the next track on the
     * release. @return true when a track was added.
     */
    bool addTrackToRelease(wxWindow* parent, Database& db, const std::string& upc);

    /** Ask for a track's position on its release. @return true when changed. */
    bool trackNumber(wxWindow* parent, Database& db, int trackId, int current);

} // namespace space::views::edit
