#include "AddTrackDialog.h"

#include <wx/statbox.h>
#include <algorithm>

// ── Construction ──────────────────────────────────────────────────────────────

AddTrackDialog::AddTrackDialog(wxWindow* parent, sqlite3* db)
    : wxDialog(parent, wxID_ANY, "Add Track",
               wxDefaultPosition, wxSize(680, 660),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m_db(db)
{
    loadData();
    buildUI();
}

AddTrackDialog::AddTrackDialog(wxWindow* parent, sqlite3* db, int trackId)
    : wxDialog(parent, wxID_ANY, "Edit Track",
               wxDefaultPosition, wxSize(680, 660),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m_db(db)
    , m_trackId(trackId)
{
    loadData();
    buildUI();
    populateForEdit();
}

// ── Data loading ──────────────────────────────────────────────────────────────

void AddTrackDialog::loadData()
{
    if (!m_db) return;

    // getAllArtists
    {
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(m_db,
            "SELECT isni, name FROM artists ORDER BY name;",
            -1, &s, nullptr) == SQLITE_OK) {
            while (sqlite3_step(s) == SQLITE_ROW) {
                const char* isni = reinterpret_cast<const char*>(sqlite3_column_text(s, 0));
                const char* name = reinterpret_cast<const char*>(sqlite3_column_text(s, 1));
                m_allArtists.push_back({ isni ? isni : "", name ? name : "" });
            }
            sqlite3_finalize(s);
        }
    }
    // getAllRecordings
    {
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(m_db,
            "SELECT isrc, name, COALESCE(version,'') FROM recordings ORDER BY name;",
            -1, &s, nullptr) == SQLITE_OK) {
            while (sqlite3_step(s) == SQLITE_ROW) {
                const char* isrc = reinterpret_cast<const char*>(sqlite3_column_text(s, 0));
                const char* name = reinterpret_cast<const char*>(sqlite3_column_text(s, 1));
                const char* ver  = reinterpret_cast<const char*>(sqlite3_column_text(s, 2));
                m_allRecordings.push_back({ isrc?isrc:"", name?name:"", ver?ver:"" });
            }
            sqlite3_finalize(s);
        }
    }
    // getAllReleases
    {
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(m_db,
            "SELECT upc, name, COALESCE(release_date,'') FROM releases ORDER BY name;",
            -1, &s, nullptr) == SQLITE_OK) {
            while (sqlite3_step(s) == SQLITE_ROW) {
                const char* upc  = reinterpret_cast<const char*>(sqlite3_column_text(s, 0));
                const char* name = reinterpret_cast<const char*>(sqlite3_column_text(s, 1));
                const char* date = reinterpret_cast<const char*>(sqlite3_column_text(s, 2));
                m_allReleases.push_back({ upc?upc:"", name?name:"", date?date:"" });
            }
            sqlite3_finalize(s);
        }
    }
}

// ── Artist combo choices helper ───────────────────────────────────────────────

wxArrayString AddTrackDialog::artistChoices() const
{
    wxArrayString choices;
    for (const auto& a : m_allArtists)
        choices.Add(wxString::FromUTF8(a.isni + "  —  " + a.name));
    return choices;
}

// ── UI construction ───────────────────────────────────────────────────────────

void AddTrackDialog::buildUI()
{
    auto* root = new wxBoxSizer(wxVERTICAL);
    root->AddSpacer(8);

    // ── Recording section ─────────────────────────────────────────────────────
    auto* recBox = new wxStaticBoxSizer(wxVERTICAL, this, "Recording");

    // ISRC combo — allows typing new or selecting existing
    {
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        row->Add(new wxStaticText(this, wxID_ANY, "ISRC:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
        wxArrayString isrcChoices;
        for (const auto& r : m_allRecordings)
            isrcChoices.Add(wxString::FromUTF8(r.isrc + "  —  " + r.name));
        m_isrcCombo = new wxComboBox(this, wxID_ANY, "", wxDefaultPosition,
                                     wxSize(220, -1), isrcChoices, wxCB_DROPDOWN);
        m_isrcCombo->SetHint("e.g. US-ABC-12-00001");
        row->Add(m_isrcCombo, 1, wxEXPAND | wxRIGHT, 10);

        row->Add(new wxStaticText(this, wxID_ANY, "Version:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
        m_recVersion = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(120, -1));
        m_recVersion->SetHint("Radio Edit");
        row->Add(m_recVersion, 0, wxEXPAND);
        recBox->Add(row, 0, wxEXPAND | wxALL, 4);
    }
    {
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        row->Add(new wxStaticText(this, wxID_ANY, "Name:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
        m_recName = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize);
        m_recName->SetHint("Recording name (required)");
        row->Add(m_recName, 1, wxEXPAND);
        recBox->Add(row, 0, wxEXPAND | wxALL, 4);
    }

    // Recording artists sub-panel
    {
        recBox->Add(new wxStaticText(this, wxID_ANY, "Recording Artists:"), 0, wxLEFT | wxTOP, 4);
        m_recArtistsPanel = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition,
                                                  wxSize(-1, 90));
        m_recArtistsPanel->SetScrollRate(0, 10);
        m_recArtistsSizer = new wxBoxSizer(wxVERTICAL);
        m_recArtistsPanel->SetSizer(m_recArtistsSizer);
        recBox->Add(m_recArtistsPanel, 0, wxEXPAND | wxALL, 4);

        auto* addBtn = new wxButton(this, wxID_ANY, "+ Add Artist", wxDefaultPosition,
                                    wxSize(-1, 24));
        addBtn->Bind(wxEVT_BUTTON, &AddTrackDialog::onAddRecArtist, this);
        recBox->Add(addBtn, 0, wxLEFT | wxBOTTOM, 6);
    }

    root->Add(recBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // ── Release section (optional) ────────────────────────────────────────────
    auto* relBox = new wxStaticBoxSizer(wxVERTICAL, this, "Release (optional)");

    {
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        row->Add(new wxStaticText(this, wxID_ANY, "UPC:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
        wxArrayString upcChoices;
        for (const auto& r : m_allReleases)
            upcChoices.Add(wxString::FromUTF8(r.upc + "  —  " + r.name));
        m_upcCombo = new wxComboBox(this, wxID_ANY, "", wxDefaultPosition,
                                    wxSize(220, -1), upcChoices, wxCB_DROPDOWN);
        m_upcCombo->SetHint("e.g. 012345678901");
        row->Add(m_upcCombo, 1, wxEXPAND | wxRIGHT, 10);

        row->Add(new wxStaticText(this, wxID_ANY, "Date:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
        m_relDate = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(100, -1));
        m_relDate->SetHint("YYYY-MM-DD");
        row->Add(m_relDate, 0, wxEXPAND);
        relBox->Add(row, 0, wxEXPAND | wxALL, 4);
    }
    {
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        row->Add(new wxStaticText(this, wxID_ANY, "Name:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
        m_relName = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize);
        m_relName->SetHint("Release name");
        row->Add(m_relName, 1, wxEXPAND);
        relBox->Add(row, 0, wxEXPAND | wxALL, 4);
    }

    // Release artists sub-panel
    {
        relBox->Add(new wxStaticText(this, wxID_ANY, "Release Artists:"), 0, wxLEFT | wxTOP, 4);
        m_relArtistsPanel = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition,
                                                  wxSize(-1, 90));
        m_relArtistsPanel->SetScrollRate(0, 10);
        m_relArtistsSizer = new wxBoxSizer(wxVERTICAL);
        m_relArtistsPanel->SetSizer(m_relArtistsSizer);
        relBox->Add(m_relArtistsPanel, 0, wxEXPAND | wxALL, 4);

        auto* addBtn = new wxButton(this, wxID_ANY, "+ Add Artist", wxDefaultPosition,
                                    wxSize(-1, 24));
        addBtn->Bind(wxEVT_BUTTON, &AddTrackDialog::onAddRelArtist, this);
        relBox->Add(addBtn, 0, wxLEFT | wxBOTTOM, 6);
    }

    root->Add(relBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // ── Dialog buttons ────────────────────────────────────────────────────────
    auto* btnRow = new wxBoxSizer(wxHORIZONTAL);
    btnRow->AddStretchSpacer();
    auto* cancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");
    auto* okBtn     = new wxButton(this, wxID_OK,     "Save");
    okBtn->SetDefault();
    cancelBtn->Bind(wxEVT_BUTTON, &AddTrackDialog::onCancel, this);
    okBtn->Bind(wxEVT_BUTTON,     &AddTrackDialog::onOK,     this);
    btnRow->Add(cancelBtn, 0, wxRIGHT, 8);
    btnRow->Add(okBtn, 0);
    root->Add(btnRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    SetSizerAndFit(root);
    SetSize(wxSize(680, 660));

    // ── Combo selection handlers ──────────────────────────────────────────────
    m_isrcCombo->Bind(wxEVT_COMBOBOX, &AddTrackDialog::onIsrcSelected, this);
    m_upcCombo->Bind(wxEVT_COMBOBOX,  &AddTrackDialog::onUpcSelected,  this);
}

// ── Edit mode pre-population ──────────────────────────────────────────────────

void AddTrackDialog::populateForEdit()
{
    if (!m_db || m_trackId < 0) return;

    // Query details directly using sqlite3 (Database::getTrackDetails needs a
    // Database object opened by path — we replicate the query inline instead).
    std::string recIsrc, recName, recVer;
    std::string relUpc, relName, relDate;

    {
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(m_db, R"(
            SELECT rec.isrc, rec.name, COALESCE(rec.version,''),
                   COALESCE(rel.upc,''), COALESCE(rel.name,''),
                   COALESCE(rel.release_date,'')
            FROM tracks t
            JOIN recordings rec ON rec.isrc = t.recording_id
            LEFT JOIN releases rel ON rel.upc = t.release_id
            WHERE t.id = ?;
        )", -1, &s, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(s, 1, m_trackId);
            if (sqlite3_step(s) == SQLITE_ROW) {
                auto col = [&](int i) -> std::string {
                    const unsigned char* t = sqlite3_column_text(s, i);
                    return t ? reinterpret_cast<const char*>(t) : "";
                };
                recIsrc = col(0); recName = col(1); recVer  = col(2);
                relUpc  = col(3); relName = col(4); relDate = col(5);
            }
            sqlite3_finalize(s);
        }
    }

    m_isrcCombo->SetValue(wxString::FromUTF8(recIsrc));
    m_recName->SetValue(wxString::FromUTF8(recName));
    m_recVersion->SetValue(wxString::FromUTF8(recVer));
    m_upcCombo->SetValue(wxString::FromUTF8(relUpc));
    m_relName->SetValue(wxString::FromUTF8(relName));
    m_relDate->SetValue(wxString::FromUTF8(relDate));

    // Recording artists
    {
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(m_db, R"(
            SELECT a.isni, a.name, ra.is_primary, ra.number
            FROM recording_artists ra
            JOIN artists a ON a.isni = ra.artist_id
            WHERE ra.recording_id = ?
            ORDER BY ra.number;
        )", -1, &s, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(s, 1, recIsrc.c_str(), -1, SQLITE_TRANSIENT);
            while (sqlite3_step(s) == SQLITE_ROW) {
                Database::ArtistRelationship ar;
                const unsigned char* isni = sqlite3_column_text(s, 0);
                const unsigned char* name = sqlite3_column_text(s, 1);
                ar.artistIsni = isni ? reinterpret_cast<const char*>(isni) : "";
                ar.artistName = name ? reinterpret_cast<const char*>(name) : "";
                ar.isPrimary  = sqlite3_column_int(s, 2) != 0;
                ar.number     = sqlite3_column_int(s, 3);
                addArtistRow(m_recArtistRows, m_recArtistsPanel, m_recArtistsSizer, &ar);
            }
            sqlite3_finalize(s);
        }
    }

    // Release artists
    if (!relUpc.empty()) {
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(m_db, R"(
            SELECT a.isni, a.name, ra.is_primary, ra.number
            FROM release_artists ra
            JOIN artists a ON a.isni = ra.artist_id
            WHERE ra.release_id = ?
            ORDER BY ra.number;
        )", -1, &s, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(s, 1, relUpc.c_str(), -1, SQLITE_TRANSIENT);
            while (sqlite3_step(s) == SQLITE_ROW) {
                Database::ArtistRelationship ar;
                const unsigned char* isni = sqlite3_column_text(s, 0);
                const unsigned char* name = sqlite3_column_text(s, 1);
                ar.artistIsni = isni ? reinterpret_cast<const char*>(isni) : "";
                ar.artistName = name ? reinterpret_cast<const char*>(name) : "";
                ar.isPrimary  = sqlite3_column_int(s, 2) != 0;
                ar.number     = sqlite3_column_int(s, 3);
                addArtistRow(m_relArtistRows, m_relArtistsPanel, m_relArtistsSizer, &ar);
            }
            sqlite3_finalize(s);
        }
    }
}

// ── Dynamic artist rows ───────────────────────────────────────────────────────

void AddTrackDialog::addArtistRow(std::vector<ArtistRow>& rows,
                                   wxScrolledWindow* panel,
                                   wxBoxSizer* sizer,
                                   const Database::ArtistRelationship* prefill)
{
    ArtistRow row;
    row.panel = new wxPanel(panel, wxID_ANY);

    auto* hs = new wxBoxSizer(wxHORIZONTAL);

    // ISNI combo
    row.isniCombo = new wxComboBox(row.panel, wxID_ANY, "", wxDefaultPosition,
                                   wxSize(180, -1), artistChoices(), wxCB_DROPDOWN);
    row.isniCombo->SetHint("ISNI");
    hs->Add(row.isniCombo, 0, wxRIGHT, 4);

    // Artist name
    row.nameCtrl = new wxTextCtrl(row.panel, wxID_ANY, "", wxDefaultPosition, wxSize(140, -1));
    row.nameCtrl->SetHint("Artist name");
    hs->Add(row.nameCtrl, 1, wxRIGHT, 4);

    // Primary checkbox
    row.primaryChk = new wxCheckBox(row.panel, wxID_ANY, "Primary");
    hs->Add(row.primaryChk, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

    // Order
    hs->Add(new wxStaticText(row.panel, wxID_ANY, "#"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
    row.orderSpin = new wxSpinCtrl(row.panel, wxID_ANY, "0", wxDefaultPosition, wxSize(55, -1),
                                   wxSP_ARROW_KEYS, 0, 99, 0);
    hs->Add(row.orderSpin, 0, wxRIGHT, 4);

    // Remove button
    row.removeBtn = new wxButton(row.panel, wxID_ANY, "✕", wxDefaultPosition, wxSize(26, 24));
    hs->Add(row.removeBtn, 0);

    row.panel->SetSizer(hs);

    // Pre-fill if editing
    if (prefill) {
        row.isniCombo->SetValue(wxString::FromUTF8(prefill->artistIsni));
        row.nameCtrl->SetValue(wxString::FromUTF8(prefill->artistName));
        row.primaryChk->SetValue(prefill->isPrimary);
        row.orderSpin->SetValue(prefill->number);
    }

    // Auto-fill name when ISNI combo selection changes
    row.isniCombo->Bind(wxEVT_COMBOBOX,
        [this, nameCtrl = row.nameCtrl](wxCommandEvent& e) {
            int sel = e.GetSelection();
            if (sel >= 0 && sel < static_cast<int>(m_allArtists.size()))
                nameCtrl->SetValue(wxString::FromUTF8(m_allArtists[sel].name));
        });

    // Remove handler — capture by pointer to identify the row
    row.removeBtn->Bind(wxEVT_BUTTON,
        [this, &rows, panel, sizer, btn = row.removeBtn](wxCommandEvent&) {
            removeArtistRow(rows, panel, sizer, btn);
        });

    sizer->Add(row.panel, 0, wxEXPAND | wxBOTTOM, 2);
    rows.push_back(row);
    refreshArtistPanel(panel, sizer);
}

void AddTrackDialog::removeArtistRow(std::vector<ArtistRow>& rows,
                                      wxScrolledWindow* panel,
                                      wxBoxSizer* sizer,
                                      wxButton* btn)
{
    auto it = std::find_if(rows.begin(), rows.end(),
                           [btn](const ArtistRow& r) { return r.removeBtn == btn; });
    if (it == rows.end()) return;

    sizer->Detach(it->panel);
    it->panel->Destroy();
    rows.erase(it);
    refreshArtistPanel(panel, sizer);
}

void AddTrackDialog::refreshArtistPanel(wxScrolledWindow* panel, wxBoxSizer* sizer)
{
    panel->FitInside();
    panel->Layout();
    Layout();
}

// ── Combo selection event handlers ───────────────────────────────────────────

void AddTrackDialog::onIsrcSelected(wxCommandEvent& e)
{
    int sel = e.GetSelection();
    if (sel >= 0 && sel < static_cast<int>(m_allRecordings.size())) {
        const auto& rec = m_allRecordings[sel];
        // Only the ISRC code portion (before "  —  ")
        m_isrcCombo->SetValue(wxString::FromUTF8(rec.isrc));
        m_recName->SetValue(wxString::FromUTF8(rec.name));
        m_recVersion->SetValue(wxString::FromUTF8(rec.version));
    }
}

void AddTrackDialog::onUpcSelected(wxCommandEvent& e)
{
    int sel = e.GetSelection();
    if (sel >= 0 && sel < static_cast<int>(m_allReleases.size())) {
        const auto& rel = m_allReleases[sel];
        m_upcCombo->SetValue(wxString::FromUTF8(rel.upc));
        m_relName->SetValue(wxString::FromUTF8(rel.name));
        m_relDate->SetValue(wxString::FromUTF8(rel.releaseDate));
    }
}

void AddTrackDialog::onIsniSelected(wxCommandEvent& e, wxTextCtrl* nameCtrl)
{
    int sel = e.GetSelection();
    if (sel >= 0 && sel < static_cast<int>(m_allArtists.size()))
        nameCtrl->SetValue(wxString::FromUTF8(m_allArtists[sel].name));
}

// ── Add/remove artist button handlers ────────────────────────────────────────

void AddTrackDialog::onAddRecArtist(wxCommandEvent&)
{
    addArtistRow(m_recArtistRows, m_recArtistsPanel, m_recArtistsSizer);
}

void AddTrackDialog::onAddRelArtist(wxCommandEvent&)
{
    addArtistRow(m_relArtistRows, m_relArtistsPanel, m_relArtistsSizer);
}

void AddTrackDialog::onRemoveRecArtist(wxCommandEvent& e)
{
    removeArtistRow(m_recArtistRows, m_recArtistsPanel, m_recArtistsSizer,
                    static_cast<wxButton*>(e.GetEventObject()));
}

void AddTrackDialog::onRemoveRelArtist(wxCommandEvent& e)
{
    removeArtistRow(m_relArtistRows, m_relArtistsPanel, m_relArtistsSizer,
                    static_cast<wxButton*>(e.GetEventObject()));
}

// ── Validation & save ─────────────────────────────────────────────────────────

std::vector<Database::ArtistRelationship> AddTrackDialog::collectArtists(
    const std::vector<ArtistRow>& rows) const
{
    std::vector<Database::ArtistRelationship> out;
    for (const auto& r : rows) {
        Database::ArtistRelationship a;
        a.artistIsni = r.isniCombo->GetValue().ToUTF8().data();
        a.artistName = r.nameCtrl->GetValue().ToUTF8().data();
        a.isPrimary  = r.primaryChk->GetValue();
        a.number     = r.orderSpin->GetValue();
        if (a.artistIsni.empty() && a.artistName.empty()) continue;
        out.push_back(a);
    }
    return out;
}

bool AddTrackDialog::validate()
{
    if (m_isrcCombo->GetValue().Trim().IsEmpty()) {
        wxMessageBox("ISRC is required for the recording.", "Validation", wxOK | wxICON_WARNING, this);
        m_isrcCombo->SetFocus();
        return false;
    }
    if (m_recName->GetValue().Trim().IsEmpty()) {
        wxMessageBox("Recording name is required.", "Validation", wxOK | wxICON_WARNING, this);
        m_recName->SetFocus();
        return false;
    }
    // If UPC is provided, release name is required
    if (!m_upcCombo->GetValue().Trim().IsEmpty() && m_relName->GetValue().Trim().IsEmpty()) {
        wxMessageBox("Release name is required when a UPC is provided.", "Validation",
                     wxOK | wxICON_WARNING, this);
        m_relName->SetFocus();
        return false;
    }
    // Each artist row must have at least an ISNI if it has a name
    auto checkArtistRows = [&](const std::vector<ArtistRow>& rows) -> bool {
        for (const auto& r : rows) {
            bool hasIsni = !r.isniCombo->GetValue().Trim().IsEmpty();
            bool hasName = !r.nameCtrl->GetValue().Trim().IsEmpty();
            if (hasName && !hasIsni) {
                wxMessageBox("Each artist must have an ISNI.", "Validation",
                             wxOK | wxICON_WARNING, this);
                r.isniCombo->SetFocus();
                return false;
            }
        }
        return true;
    };
    if (!checkArtistRows(m_recArtistRows)) return false;
    if (!checkArtistRows(m_relArtistRows)) return false;
    return true;
}

bool AddTrackDialog::save()
{
    if (!m_db) return false;

    std::string isrc    = m_isrcCombo->GetValue().Trim().ToUTF8().data();
    std::string recName = m_recName->GetValue().Trim().ToUTF8().data();
    std::string recVer  = m_recVersion->GetValue().Trim().ToUTF8().data();
    std::string upc     = m_upcCombo->GetValue().Trim().ToUTF8().data();
    std::string relName = m_relName->GetValue().Trim().ToUTF8().data();
    std::string relDate = m_relDate->GetValue().Trim().ToUTF8().data();

    auto recArtists = collectArtists(m_recArtistRows);
    auto relArtists = collectArtists(m_relArtistRows);

    // Upsert artists first (FK requirement for recording_artists / release_artists)
    auto upsertArtist = [&](const std::string& artistIsni, const std::string& artistName) {
        if (artistIsni.empty()) return;
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(m_db,
            "INSERT INTO artists (isni, name) VALUES (?, ?)"
            "  ON CONFLICT(isni) DO UPDATE SET name = excluded.name;",
            -1, &s, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(s, 1, artistIsni.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(s, 2, artistName.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(s);
            sqlite3_finalize(s);
        }
    };
    for (const auto& a : recArtists) upsertArtist(a.artistIsni, a.artistName);
    for (const auto& a : relArtists) upsertArtist(a.artistIsni, a.artistName);

    // Upsert recording
    {
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(m_db,
            "INSERT INTO recordings (isrc, name, version) VALUES (?, ?, ?)"
            "  ON CONFLICT(isrc) DO UPDATE SET name = excluded.name, version = excluded.version;",
            -1, &s, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text(s, 1, isrc.c_str(),    -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(s, 2, recName.c_str(), -1, SQLITE_TRANSIENT);
        if (recVer.empty()) sqlite3_bind_null(s, 3);
        else sqlite3_bind_text(s, 3, recVer.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(s);
        sqlite3_finalize(s);
    }

    // Set recording artists
    {
        sqlite3_stmt* del = nullptr;
        sqlite3_prepare_v2(m_db, "DELETE FROM recording_artists WHERE recording_id = ?;",
                           -1, &del, nullptr);
        sqlite3_bind_text(del, 1, isrc.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(del);
        sqlite3_finalize(del);

        sqlite3_stmt* ins = nullptr;
        if (sqlite3_prepare_v2(m_db,
            "INSERT INTO recording_artists (recording_id, artist_id, is_primary, number)"
            "  VALUES (?, ?, ?, ?);",
            -1, &ins, nullptr) == SQLITE_OK) {
            for (const auto& a : recArtists) {
                sqlite3_reset(ins);
                sqlite3_bind_text(ins, 1, isrc.c_str(),         -1, SQLITE_TRANSIENT);
                sqlite3_bind_text(ins, 2, a.artistIsni.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_int(ins,  3, a.isPrimary ? 1 : 0);
                sqlite3_bind_int(ins,  4, a.number);
                sqlite3_step(ins);
            }
            sqlite3_finalize(ins);
        }
    }

    // Upsert release (if provided)
    if (!upc.empty()) {
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(m_db,
            "INSERT INTO releases (upc, name, release_date) VALUES (?, ?, ?)"
            "  ON CONFLICT(upc) DO UPDATE SET name = excluded.name,"
            "    release_date = excluded.release_date;",
            -1, &s, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(s, 1, upc.c_str(),     -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(s, 2, relName.c_str(), -1, SQLITE_TRANSIENT);
            if (relDate.empty()) sqlite3_bind_null(s, 3);
            else sqlite3_bind_text(s, 3, relDate.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(s);
            sqlite3_finalize(s);
        }

        // Set release artists
        {
            sqlite3_stmt* del = nullptr;
            sqlite3_prepare_v2(m_db, "DELETE FROM release_artists WHERE release_id = ?;",
                               -1, &del, nullptr);
            sqlite3_bind_text(del, 1, upc.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(del);
            sqlite3_finalize(del);

            sqlite3_stmt* ins = nullptr;
            if (sqlite3_prepare_v2(m_db,
                "INSERT INTO release_artists (release_id, artist_id, number, is_primary)"
                "  VALUES (?, ?, ?, ?);",
                -1, &ins, nullptr) == SQLITE_OK) {
                for (const auto& a : relArtists) {
                    sqlite3_reset(ins);
                    sqlite3_bind_text(ins, 1, upc.c_str(),          -1, SQLITE_TRANSIENT);
                    sqlite3_bind_text(ins, 2, a.artistIsni.c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_bind_int(ins,  3, a.number);
                    sqlite3_bind_int(ins,  4, a.isPrimary ? 1 : 0);
                    sqlite3_step(ins);
                }
                sqlite3_finalize(ins);
            }
        }
    }

    // Find or create track
    int64_t trackId = -1;
    {
        const char* selectSql = upc.empty()
            ? "SELECT id FROM tracks WHERE recording_id = ? AND release_id IS NULL LIMIT 1;"
            : "SELECT id FROM tracks WHERE recording_id = ? AND release_id = ? LIMIT 1;";
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(m_db, selectSql, -1, &s, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text(s, 1, isrc.c_str(), -1, SQLITE_TRANSIENT);
        if (!upc.empty())
            sqlite3_bind_text(s, 2, upc.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(s) == SQLITE_ROW)
            trackId = sqlite3_column_int64(s, 0);
        sqlite3_finalize(s);

        if (trackId < 0) {
            sqlite3_stmt* ins = nullptr;
            if (sqlite3_prepare_v2(m_db,
                "INSERT INTO tracks (recording_id, release_id) VALUES (?, ?);",
                -1, &ins, nullptr) != SQLITE_OK) return false;
            sqlite3_bind_text(ins, 1, isrc.c_str(), -1, SQLITE_TRANSIENT);
            if (upc.empty()) sqlite3_bind_null(ins, 2);
            else sqlite3_bind_text(ins, 2, upc.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(ins);
            trackId = sqlite3_last_insert_rowid(m_db);
            sqlite3_finalize(ins);
        }
    }

    if (trackId < 0) return false;

    // In edit mode we only update the track record itself (already done above via upserts).
    // In add mode we also ensure a save record exists.
    if (m_trackId < 0) {
        // Reactivate soft-deleted save or insert new
        int64_t saveId = -1;
        {
            sqlite3_stmt* s = nullptr;
            if (sqlite3_prepare_v2(m_db,
                "SELECT id FROM saves WHERE track_id = ? AND deleted_at IS NULL LIMIT 1;",
                -1, &s, nullptr) == SQLITE_OK) {
                sqlite3_bind_int64(s, 1, trackId);
                if (sqlite3_step(s) == SQLITE_ROW)
                    saveId = sqlite3_column_int64(s, 0);
                sqlite3_finalize(s);
            }
        }
        if (saveId < 0) {
            sqlite3_stmt* s = nullptr;
            if (sqlite3_prepare_v2(m_db,
                "SELECT id FROM saves WHERE track_id = ? AND deleted_at IS NOT NULL LIMIT 1;",
                -1, &s, nullptr) == SQLITE_OK) {
                sqlite3_bind_int64(s, 1, trackId);
                if (sqlite3_step(s) == SQLITE_ROW) {
                    int64_t id = sqlite3_column_int64(s, 0);
                    sqlite3_finalize(s);
                    sqlite3_stmt* upd = nullptr;
                    if (sqlite3_prepare_v2(m_db,
                        "UPDATE saves SET deleted_at = NULL, saved_at = datetime('now') WHERE id = ?;",
                        -1, &upd, nullptr) == SQLITE_OK) {
                        sqlite3_bind_int64(upd, 1, id);
                        sqlite3_step(upd);
                        sqlite3_finalize(upd);
                    }
                } else {
                    sqlite3_finalize(s);
                    sqlite3_stmt* ins = nullptr;
                    if (sqlite3_prepare_v2(m_db, "INSERT INTO saves (track_id) VALUES (?);",
                                           -1, &ins, nullptr) == SQLITE_OK) {
                        sqlite3_bind_int64(ins, 1, trackId);
                        sqlite3_step(ins);
                        sqlite3_finalize(ins);
                    }
                }
            }
        }
    }

    return true;
}

// ── Dialog button handlers ────────────────────────────────────────────────────

void AddTrackDialog::onOK(wxCommandEvent&)
{
    if (!validate()) return;
    if (!save()) {
        wxMessageBox("Failed to save the track. Check the database.", "Error",
                     wxOK | wxICON_ERROR, this);
        return;
    }
    EndModal(wxID_OK);
}

void AddTrackDialog::onCancel(wxCommandEvent&)
{
    EndModal(wxID_CANCEL);
}
