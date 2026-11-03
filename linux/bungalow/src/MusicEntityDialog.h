#pragma once

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/checklst.h>

#include <string>
#include <vector>

#include "Database.h"

/**
 * MusicEntityDialog — modal add/edit form for an artist, recording or release.
 *
 * Shows one text field per Field and, when @p allArtists is given, a checklist
 * of library artists to credit plus a "New artists" field for names not in the
 * library yet. Credit order is the checklist order: previously credited artists
 * come first (in credit order), and the first checked artist is primary.
 *
 * Required fields are validated on OK; the caller persists the values.
 */
class MusicEntityDialog : public wxDialog {
public:
    struct Field {
        std::string label;
        std::string value;
        std::string hint;
        bool        required = false;
        bool        readOnly = false;
    };

    MusicEntityDialog(wxWindow* parent, const std::string& title, std::vector<Field> fields,
                      const std::vector<Database::ArtistRecord>* allArtists = nullptr,
                      const std::vector<Database::ArtistRelationship>& credited = {});
    ~MusicEntityDialog() override = default;

    /** Trimmed UTF-8 value of field @p index. */
    std::string value(size_t index) const;

    /**
     * The credits picked in the dialog, numbered in order with the first one
     * primary. New names reuse an existing artist with the same name
     * (case-insensitive) or are created in @p db with a local id.
     */
    std::vector<Database::ArtistRelationship> collectArtists(Database& db) const;

private:
    void onOK(wxCommandEvent& e);

    std::vector<Field>       m_fields;
    std::vector<wxTextCtrl*> m_ctrls;

    std::vector<Database::ArtistRecord> m_artists; // parallel to m_artistList items
    wxCheckListBox* m_artistList = nullptr;
    wxTextCtrl*     m_newArtists = nullptr;
};
