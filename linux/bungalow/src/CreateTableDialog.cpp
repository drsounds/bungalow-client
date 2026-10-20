#include "CreateTableDialog.h"

#include <wx/stattext.h>
#include <wx/sizer.h>

#include <regex>

// ── Construction ──────────────────────────────────────────────────────────────

CreateTableDialog::CreateTableDialog(wxWindow* parent, sqlite3* db)
    : wxDialog(parent, wxID_ANY, "Create Custom Table",
               wxDefaultPosition, wxSize(500, 620),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , m_db(db)
{
    buildUI();
    addColRow(); // start with one blank column row
    Centre();
}

// ── UI construction ───────────────────────────────────────────────────────────

void CreateTableDialog::buildUI()
{
    auto* root = new wxBoxSizer(wxVERTICAL);
    SetSizer(root);

    // ── Table metadata ────────────────────────────────────────────────────────
    auto* metaBox = new wxStaticBoxSizer(wxVERTICAL, this, "Table Definition");

    auto* metaGrid = new wxFlexGridSizer(2, 8, 8);
    metaGrid->AddGrowableCol(1);

    auto addRow = [&](const char* label, wxTextCtrl*& out, bool multiLine = false) {
        metaGrid->Add(new wxStaticText(this, wxID_ANY, label), 0,
                      wxALIGN_CENTER_VERTICAL);
        long style = multiLine ? wxTE_MULTILINE : 0;
        out = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, style);
        if (multiLine) out->SetMinSize(wxSize(-1, 48));
        metaGrid->Add(out, 1, wxEXPAND);
    };

    addRow("Table name:",       m_nameCtrl);
    addRow("Display name:",     m_displayNameCtrl);
    addRow("Noun (singular):",  m_nounSingularCtrl);
    addRow("Noun (plural):",    m_nounPluralCtrl);
    addRow("Description:",      m_descriptionCtrl, true);

    metaBox->Add(metaGrid, 0, wxEXPAND | wxALL, 8);
    root->Add(metaBox, 0, wxEXPAND | wxALL, 10);

    // ── Columns header ────────────────────────────────────────────────────────
    root->Add(new wxStaticText(this, wxID_ANY, "Columns:"), 0, wxLEFT | wxBOTTOM, 10);

    // Scrollable area for column rows
    m_colsPanel = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition,
                                       wxSize(-1, 200), wxVSCROLL);
    m_colsPanel->SetScrollRate(0, 20);
    m_colsSizer = new wxBoxSizer(wxVERTICAL);
    m_colsPanel->SetSizer(m_colsSizer);
    root->Add(m_colsPanel, 1, wxEXPAND | wxLEFT | wxRIGHT, 10);

    // "+ Add Column" button
    auto* addColBtn = new wxButton(this, wxID_ANY, "+ Add Column");
    addColBtn->Bind(wxEVT_BUTTON, &CreateTableDialog::onAddCol, this);
    root->Add(addColBtn, 0, wxLEFT | wxTOP, 10);

    // Standard OK / Cancel
    root->AddStretchSpacer();
    auto* btnSizer = CreateButtonSizer(wxOK | wxCANCEL);
    root->Add(btnSizer, 0, wxEXPAND | wxALL, 10);

    Bind(wxEVT_BUTTON, &CreateTableDialog::onOK,     this, wxID_OK);
    Bind(wxEVT_BUTTON, &CreateTableDialog::onCancel, this, wxID_CANCEL);
}

// ── Column row management ─────────────────────────────────────────────────────

void CreateTableDialog::addColRow(const std::string& name, const std::string& type)
{
    ColRow cr;
    cr.panel = new wxPanel(m_colsPanel, wxID_ANY);
    auto* rowSizer = new wxBoxSizer(wxHORIZONTAL);
    cr.panel->SetSizer(rowSizer);

    // Column name field
    cr.nameCtrl = new wxTextCtrl(cr.panel, wxID_ANY, name);
    rowSizer->Add(cr.nameCtrl, 1, wxRIGHT, 6);

    // Type dropdown — TEXT / INTEGER / REAL / BELONGS_TO
    wxArrayString types;
    types.Add("TEXT");
    types.Add("INTEGER");
    types.Add("REAL");
    types.Add("BELONGS_TO");
    cr.typeCtrl = new wxChoice(cr.panel, wxID_ANY, wxDefaultPosition, wxSize(110, -1), types);

    int sel = 0;
    std::string targetTable;
    if (type == "INTEGER")      sel = 1;
    else if (type == "REAL")    sel = 2;
    else if (type.rfind("BELONGS_TO:", 0) == 0) {
        sel = 3;
        targetTable = type.substr(11); // e.g. "artists"
    }
    cr.typeCtrl->SetSelection(sel);
    rowSizer->Add(cr.typeCtrl, 0, wxRIGHT, 6);

    // Target table field — visible only for BELONGS_TO
    cr.targetCtrl = new wxTextCtrl(cr.panel, wxID_ANY, targetTable,
                                   wxDefaultPosition, wxSize(100, -1));
    cr.targetCtrl->SetHint("target table");
    cr.targetCtrl->Show(sel == 3);
    rowSizer->Add(cr.targetCtrl, 0, wxRIGHT, 6);

    // Show/hide targetCtrl when type changes
    cr.typeCtrl->Bind(wxEVT_CHOICE, [this, cr](wxCommandEvent& /*e*/) mutable {
        bool isBelongsTo = (cr.typeCtrl->GetStringSelection() == "BELONGS_TO");
        cr.targetCtrl->Show(isBelongsTo);
        refreshColsPanel();
    });

    // Remove button
    cr.removeBtn = new wxButton(cr.panel, wxID_ANY, "✕",
                                wxDefaultPosition, wxSize(28, -1));
    cr.removeBtn->Bind(wxEVT_BUTTON, &CreateTableDialog::onRemoveCol, this);
    rowSizer->Add(cr.removeBtn, 0);

    m_colRows.push_back(cr);
    m_colsSizer->Add(cr.panel, 0, wxEXPAND | wxBOTTOM, 4);
    refreshColsPanel();
}

void CreateTableDialog::removeColRow(wxButton* btn)
{
    for (auto it = m_colRows.begin(); it != m_colRows.end(); ++it) {
        if (it->removeBtn == btn) {
            m_colsSizer->Detach(it->panel);
            it->panel->Destroy();
            m_colRows.erase(it);
            break;
        }
    }
    refreshColsPanel();
}

void CreateTableDialog::refreshColsPanel()
{
    m_colsPanel->FitInside();
    m_colsPanel->Layout();
    m_colsPanel->Refresh();
    Layout();
}

// ── Validation & save ─────────────────────────────────────────────────────────

bool CreateTableDialog::validate()
{
    std::string name = m_nameCtrl->GetValue().ToStdString();
    if (name.empty()) {
        wxMessageBox("Table name cannot be empty.", "Validation", wxOK | wxICON_WARNING, this);
        return false;
    }
    static const std::regex kNameRe("^[A-Za-z][A-Za-z0-9_]*$");
    if (!std::regex_match(name, kNameRe)) {
        wxMessageBox("Table name must start with a letter and contain only\n"
                     "letters, digits, or underscores.",
                     "Validation", wxOK | wxICON_WARNING, this);
        return false;
    }
    if (m_colRows.empty()) {
        wxMessageBox("Add at least one column.", "Validation", wxOK | wxICON_WARNING, this);
        return false;
    }
    for (auto& cr : m_colRows) {
        std::string cname = cr.nameCtrl->GetValue().ToStdString();
        if (cname.empty()) {
            wxMessageBox("All column names must be non-empty.", "Validation",
                         wxOK | wxICON_WARNING, this);
            return false;
        }
        static const std::regex kColRe("^[A-Za-z][A-Za-z0-9_]*$");
        if (!std::regex_match(cname, kColRe)) {
            wxMessageBox("Column name '" + cname + "' is not valid.\n"
                         "Use letters, digits, or underscores (start with a letter).",
                         "Validation", wxOK | wxICON_WARNING, this);
            return false;
        }
        if (cr.typeCtrl->GetStringSelection() == "BELONGS_TO") {
            std::string target = cr.targetCtrl->GetValue().ToStdString();
            if (target.empty()) {
                wxMessageBox("Column '" + cname + "' is a relation — enter the target table name.",
                             "Validation", wxOK | wxICON_WARNING, this);
                return false;
            }
        }
    }
    return true;
}

bool CreateTableDialog::save()
{
    if (!m_db) return false;

    std::string name         = m_nameCtrl->GetValue().ToStdString();
    std::string displayName  = m_displayNameCtrl->GetValue().ToStdString();
    std::string nounSingular = m_nounSingularCtrl->GetValue().ToStdString();
    std::string nounPlural   = m_nounPluralCtrl->GetValue().ToStdString();
    std::string description  = m_descriptionCtrl->GetValue().ToStdString();

    // Build column list — for BELONGS_TO encode type as "BELONGS_TO:<target>"
    std::vector<std::pair<std::string,std::string>> cols;
    for (auto& cr : m_colRows) {
        std::string cname = cr.nameCtrl->GetValue().ToStdString();
        std::string ctype = cr.typeCtrl->GetStringSelection().ToStdString();
        if (ctype == "BELONGS_TO") {
            std::string target = cr.targetCtrl->GetValue().ToStdString();
            ctype = "BELONGS_TO:" + target;
        }
        if (ctype.empty()) ctype = "TEXT";
        cols.emplace_back(cname, ctype);
    }

    // Encode columns
    std::string encoded;
    for (size_t i = 0; i < cols.size(); ++i) {
        if (i) encoded += '|';
        encoded += cols[i].first + ':' + cols[i].second;
    }

    // Insert meta row with all metadata
    {
        sqlite3_stmt* ins = nullptr;
        if (sqlite3_prepare_v2(m_db,
            "INSERT INTO db_tables"
            " (name, columns, display_name, noun_singular, noun_plural, description)"
            " VALUES (?, ?, ?, ?, ?, ?);",
            -1, &ins, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text(ins, 1, name.c_str(),         -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(ins, 2, encoded.c_str(),      -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(ins, 3, displayName.c_str(),  -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(ins, 4, nounSingular.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(ins, 5, nounPlural.c_str(),   -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(ins, 6, description.c_str(),  -1, SQLITE_TRANSIENT);
        int rc = sqlite3_step(ins);
        sqlite3_finalize(ins);
        if (rc != SQLITE_DONE) {
            wxMessageBox("A table named '" + name + "' already exists.",
                         "Error", wxOK | wxICON_ERROR, this);
            return false;
        }
    }

    // Create physical table — UUID4 TEXT primary key; BELONGS_TO stored as TEXT
    std::string ddl = "CREATE TABLE IF NOT EXISTS user_" + name +
                      " (id TEXT PRIMARY KEY";
    for (auto& col : cols) {
        std::string safeType = "TEXT";
        if (col.second == "INTEGER") safeType = "INTEGER";
        else if (col.second == "REAL") safeType = "REAL";
        // BELONGS_TO:<target> → TEXT in SQLite
        ddl += ", " + col.first + " " + safeType;
    }
    ddl += ");";
    char* errmsg = nullptr;
    sqlite3_exec(m_db, ddl.c_str(), nullptr, nullptr, &errmsg);
    if (errmsg) {
        wxMessageBox(std::string("SQLite error: ") + errmsg, "Error",
                     wxOK | wxICON_ERROR, this);
        sqlite3_free(errmsg);
        sqlite3_exec(m_db, ("DELETE FROM db_tables WHERE name='" + name + "';").c_str(),
                     nullptr, nullptr, nullptr);
        return false;
    }

    m_createdName = name;
    return true;
}

// ── Event handlers ────────────────────────────────────────────────────────────

void CreateTableDialog::onAddCol(wxCommandEvent& /*e*/)
{
    addColRow();
}

void CreateTableDialog::onRemoveCol(wxCommandEvent& e)
{
    removeColRow(dynamic_cast<wxButton*>(e.GetEventObject()));
}

void CreateTableDialog::onOK(wxCommandEvent& /*e*/)
{
    if (!validate()) return;
    if (!save()) return;
    EndModal(wxID_OK);
}

void CreateTableDialog::onCancel(wxCommandEvent& /*e*/)
{
    EndModal(wxID_CANCEL);
}
