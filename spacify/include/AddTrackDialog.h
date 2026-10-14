#pragma once

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/spinctrl.h>
#include <wx/scrolwin.h>

#include <sqlite3.h>
#include <string>
#include <vector>

#include "Database.h"

/**
 * AddTrackDialog — modal form for manually adding or editing a library track.
 *
 * Add mode:  AddTrackDialog(parent, db)
 * Edit mode: AddTrackDialog(parent, db, trackId)
 *
 * On wxID_OK the caller is responsible for refreshing the library table.
 * The dialog inserts/updates all required records (artists, recording, release,
 * track, save) and saves them to the database before closing.
 */
class AddTrackDialog : public wxDialog {
public:
    explicit AddTrackDialog(wxWindow* parent, sqlite3* db);
    AddTrackDialog(wxWindow* parent, sqlite3* db, int trackId);
    ~AddTrackDialog() override = default;

private:
    // ── State ─────────────────────────────────────────────────────────────────
    sqlite3* m_db;
    int      m_trackId = -1;   // -1 = add mode

    std::vector<Database::ArtistRecord>    m_allArtists;
    std::vector<Database::RecordingRecord> m_allRecordings;
    std::vector<Database::ReleaseRecord>   m_allReleases;

    // ── Recording section ─────────────────────────────────────────────────────
    wxComboBox* m_isrcCombo   = nullptr;
    wxTextCtrl* m_recName     = nullptr;
    wxTextCtrl* m_recVersion  = nullptr;

    // ── Release section ───────────────────────────────────────────────────────
    wxComboBox* m_upcCombo    = nullptr;
    wxTextCtrl* m_relName     = nullptr;
    wxTextCtrl* m_relDate     = nullptr;

    // ── Dynamic artist rows ───────────────────────────────────────────────────
    struct ArtistRow {
        wxPanel*    panel      = nullptr;
        wxComboBox* isniCombo  = nullptr;
        wxTextCtrl* nameCtrl   = nullptr;
        wxCheckBox* primaryChk = nullptr;
        wxSpinCtrl* orderSpin  = nullptr;
        wxButton*   removeBtn  = nullptr;
    };

    std::vector<ArtistRow> m_recArtistRows;
    wxScrolledWindow*      m_recArtistsPanel = nullptr;
    wxBoxSizer*            m_recArtistsSizer = nullptr;

    std::vector<ArtistRow> m_relArtistRows;
    wxScrolledWindow*      m_relArtistsPanel = nullptr;
    wxBoxSizer*            m_relArtistsSizer = nullptr;

    // ── Setup ─────────────────────────────────────────────────────────────────
    void buildUI();
    void loadData();
    void populateForEdit();
    wxArrayString artistChoices() const;

    // ── Dynamic artist row helpers ────────────────────────────────────────────
    void addArtistRow(std::vector<ArtistRow>& rows,
                      wxScrolledWindow* panel,
                      wxBoxSizer* sizer,
                      const Database::ArtistRelationship* prefill = nullptr);
    void removeArtistRow(std::vector<ArtistRow>& rows,
                         wxScrolledWindow* panel,
                         wxBoxSizer* sizer,
                         wxButton* btn);
    void refreshArtistPanel(wxScrolledWindow* panel, wxBoxSizer* sizer);

    // ── Event handlers ────────────────────────────────────────────────────────
    void onIsrcSelected(wxCommandEvent& e);
    void onUpcSelected(wxCommandEvent& e);
    void onIsniSelected(wxCommandEvent& e, wxTextCtrl* nameCtrl);
    void onAddRecArtist(wxCommandEvent& e);
    void onAddRelArtist(wxCommandEvent& e);
    void onRemoveRecArtist(wxCommandEvent& e);
    void onRemoveRelArtist(wxCommandEvent& e);
    void onOK(wxCommandEvent& e);
    void onCancel(wxCommandEvent& e);

    // ── Save logic ────────────────────────────────────────────────────────────
    bool validate();
    bool save();
    std::vector<Database::ArtistRelationship> collectArtists(
        const std::vector<ArtistRow>& rows) const;
};
