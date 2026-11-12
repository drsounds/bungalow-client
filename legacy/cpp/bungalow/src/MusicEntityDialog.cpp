#include "MusicEntityDialog.h"

#include <algorithm>
#include <cctype>

static std::string trimmed(const wxString& s) {
    wxString v = s;
    v.Trim(true).Trim(false);
    return std::string(v.ToUTF8().data());
}

static std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// "Name" for local artists, "Name  —  ISNI" when there is a real identifier.
static wxString artistLabel(const Database::ArtistRecord& a) {
    if (a.isni.rfind("local:", 0) == 0) return wxString::FromUTF8(a.name);
    return wxString::FromUTF8(a.name + "  —  " + a.isni);
}

// ── Construction ──────────────────────────────────────────────────────────────

MusicEntityDialog::MusicEntityDialog(wxWindow* parent, const std::string& title,
                                     std::vector<Field> fields,
                                     const std::vector<Database::ArtistRecord>* allArtists,
                                     const std::vector<Database::ArtistRelationship>& credited)
    : wxDialog(parent, wxID_ANY, wxString::FromUTF8(title),
               wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m_fields(std::move(fields))
{
    auto* root = new wxBoxSizer(wxVERTICAL);

    auto* grid = new wxFlexGridSizer(2, 8, 8);
    grid->AddGrowableCol(1, 1);
    for (const auto& f : m_fields) {
        grid->Add(new wxStaticText(this, wxID_ANY,
                      wxString::FromUTF8(f.label + (f.required ? " *" : "") + ":")),
                  0, wxALIGN_CENTER_VERTICAL);
        auto* ctrl = new wxTextCtrl(this, wxID_ANY, wxString::FromUTF8(f.value),
                                    wxDefaultPosition, wxSize(340, -1));
        if (!f.hint.empty()) ctrl->SetHint(wxString::FromUTF8(f.hint));
        if (f.readOnly) ctrl->SetEditable(false);
        grid->Add(ctrl, 1, wxEXPAND);
        m_ctrls.push_back(ctrl);
    }
    root->Add(grid, 0, wxEXPAND | wxALL, 12);

    if (allArtists) {
        // Credited artists first, in credit order, then everyone else by name.
        wxArrayString labels;
        for (const auto& c : credited) {
            m_artists.push_back({ c.artistIsni, c.artistName });
            labels.Add(artistLabel(m_artists.back()));
        }
        for (const auto& a : *allArtists) {
            bool isCredited = std::any_of(credited.begin(), credited.end(),
                [&a](const Database::ArtistRelationship& c) { return c.artistIsni == a.isni; });
            if (isCredited) continue;
            m_artists.push_back(a);
            labels.Add(artistLabel(a));
        }

        root->Add(new wxStaticText(this, wxID_ANY, "Artists (the first checked is primary):"),
                  0, wxLEFT | wxRIGHT, 12);
        m_artistList = new wxCheckListBox(this, wxID_ANY, wxDefaultPosition,
                                          wxSize(-1, 160), labels);
        for (size_t i = 0; i < credited.size(); ++i)
            m_artistList->Check(static_cast<unsigned int>(i), true);
        root->Add(m_artistList, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);

        auto* row = new wxBoxSizer(wxHORIZONTAL);
        row->Add(new wxStaticText(this, wxID_ANY, "New artists:"),
                 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
        m_newArtists = new wxTextCtrl(this, wxID_ANY, "");
        m_newArtists->SetHint("Comma-separated names not in the library yet");
        row->Add(m_newArtists, 1, wxEXPAND);
        root->Add(row, 0, wxEXPAND | wxALL, 12);
    }

    root->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 12);
    SetSizerAndFit(root);
    CentreOnParent();

    Bind(wxEVT_BUTTON, &MusicEntityDialog::onOK, this, wxID_OK);
}

// ── Values ────────────────────────────────────────────────────────────────────

std::string MusicEntityDialog::value(size_t index) const {
    if (index >= m_ctrls.size()) return "";
    return trimmed(m_ctrls[index]->GetValue());
}

std::vector<Database::ArtistRelationship>
MusicEntityDialog::collectArtists(Database& db) const {
    std::vector<Database::ArtistRelationship> out;
    if (!m_artistList) return out;

    auto add = [&out](const std::string& isni, const std::string& name) {
        for (const auto& c : out)
            if (c.artistIsni == isni) return;
        Database::ArtistRelationship r;
        r.artistIsni = isni;
        r.artistName = name;
        r.number     = static_cast<int>(out.size());
        r.isPrimary  = out.empty();
        out.push_back(r);
    };

    for (unsigned int i = 0; i < m_artistList->GetCount(); ++i)
        if (m_artistList->IsChecked(i))
            add(m_artists[i].isni, m_artists[i].name);

    std::string pending = trimmed(m_newArtists->GetValue());
    size_t pos = 0;
    while (pos <= pending.size()) {
        size_t comma = pending.find(',', pos);
        std::string name = trimmed(wxString::FromUTF8(
            pending.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos)));
        bool collected = std::any_of(out.begin(), out.end(),
            [&name](const Database::ArtistRelationship& c) { return lower(c.artistName) == lower(name); });
        if (!name.empty() && !collected) {
            auto existing = std::find_if(m_artists.begin(), m_artists.end(),
                [&name](const Database::ArtistRecord& a) { return lower(a.name) == lower(name); });
            if (existing != m_artists.end()) {
                add(existing->isni, existing->name);
            } else {
                std::string isni = Database::generateLocalId();
                db.upsertArtist(isni, name);
                add(isni, name);
            }
        }
        if (comma == std::string::npos) break;
        pos = comma + 1;
    }
    return out;
}

// ── Validation ────────────────────────────────────────────────────────────────

void MusicEntityDialog::onOK(wxCommandEvent& e) {
    for (size_t i = 0; i < m_fields.size(); ++i) {
        if (m_fields[i].required && value(i).empty()) {
            wxMessageBox(wxString::FromUTF8(m_fields[i].label + " is required."),
                         "Missing value", wxOK | wxICON_WARNING, this);
            m_ctrls[i]->SetFocus();
            return;
        }
    }
    e.Skip(); // let the default handler close the dialog with wxID_OK
}
