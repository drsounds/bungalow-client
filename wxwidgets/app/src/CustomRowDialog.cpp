#include "CustomRowDialog.h"

#include <wx/stattext.h>
#include <wx/sizer.h>
#include <random>
#include <cstdio>

// File-scope UUID4 generator (mirrors the one in Database.cpp)
static std::string dialogGenerateUUID4()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFFu);
    uint32_t a = dist(gen), b = dist(gen), c = dist(gen), e = dist(gen);
    b = (b & 0xFFFF0FFFu) | 0x00004000u;
    c = (c & 0x3FFFFFFFu) | 0x80000000u;
    char buf[37];
    std::snprintf(buf, sizeof(buf), "%08x-%04x-%04x-%04x-%04x%08x",
                  a, (b>>16)&0xFFFF, b&0xFFFF, (c>>16)&0xFFFF, c&0xFFFF, e);
    return buf;
}


// ── Constructors ──────────────────────────────────────────────────────────────

CustomRowDialog::CustomRowDialog(wxWindow* parent, sqlite3* db,
                                 const std::string& tableName)
    : wxDialog(parent, wxID_ANY, "Add Row — " + tableName,
               wxDefaultPosition, wxSize(420, -1),
               wxDEFAULT_DIALOG_STYLE)
    , m_db(db)
    , m_tableName(tableName)
{
    buildUI();
    Fit();
    Centre();
}

CustomRowDialog::CustomRowDialog(wxWindow* parent, sqlite3* db,
                                 const std::string& tableName, const std::string& rowUuid)
    : wxDialog(parent, wxID_ANY, "Edit Row — " + tableName,
               wxDefaultPosition, wxSize(420, -1),
               wxDEFAULT_DIALOG_STYLE)
    , m_db(db)
    , m_tableName(tableName)
    , m_rowUuid(rowUuid)
{
    buildUI();
    populateForEdit();
    Fit();
    Centre();
}

// ── UI construction ───────────────────────────────────────────────────────────

void CustomRowDialog::buildUI()
{
    // Load column definitions directly from db_tables
    {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(m_db,
            "SELECT columns FROM db_tables WHERE name = ? LIMIT 1;",
            -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, m_tableName.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                const unsigned char* c = sqlite3_column_text(stmt, 0);
                std::string encoded = c ? reinterpret_cast<const char*>(c) : "";
                // Decode pipe-separated "name:type|name:type"
                size_t pos = 0;
                while (pos <= encoded.size()) {
                    size_t pipe  = encoded.find('|', pos);
                    std::string token = (pipe == std::string::npos)
                        ? encoded.substr(pos) : encoded.substr(pos, pipe - pos);
                    size_t colon = token.find(':');
                    if (colon != std::string::npos)
                        m_def.columns.emplace_back(token.substr(0, colon),
                                                   token.substr(colon + 1));
                    if (pipe == std::string::npos) break;
                    pos = pipe + 1;
                }
            }
            sqlite3_finalize(stmt);
        }
    }
    m_def.name = m_tableName;

    auto* root = new wxBoxSizer(wxVERTICAL);
    SetSizer(root);

    auto* grid = new wxFlexGridSizer(2, 8, 8);
    grid->AddGrowableCol(1);

    for (auto& col : m_def.columns) {
        std::string label = col.first + " (" + col.second + ")";
        grid->Add(new wxStaticText(this, wxID_ANY, label + ":"), 0,
                  wxALIGN_CENTER_VERTICAL);
        auto* ctrl = new wxTextCtrl(this, wxID_ANY);
        grid->Add(ctrl, 1, wxEXPAND);
        m_fields.push_back(ctrl);
    }

    root->Add(grid, 0, wxEXPAND | wxALL, 12);

    auto* btnSizer = CreateButtonSizer(wxOK | wxCANCEL);
    root->Add(btnSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    Bind(wxEVT_BUTTON, &CustomRowDialog::onOK,     this, wxID_OK);
    Bind(wxEVT_BUTTON, &CustomRowDialog::onCancel, this, wxID_CANCEL);
}

// ── Edit prefill ──────────────────────────────────────────────────────────────

void CustomRowDialog::populateForEdit()
{
    if (m_rowUuid.empty() || m_def.columns.empty()) return;

    std::string sql = "SELECT * FROM user_" + m_tableName + " WHERE id=? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_text(stmt, 1, m_rowUuid.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int ncols = sqlite3_column_count(stmt);
        // column 0 = id, columns 1..N = user columns
        for (int i = 1; i < ncols && (i - 1) < static_cast<int>(m_fields.size()); ++i) {
            const unsigned char* t = sqlite3_column_text(stmt, i);
            if (t) m_fields[i - 1]->SetValue(reinterpret_cast<const char*>(t));
        }
    }
    sqlite3_finalize(stmt);
}

// ── Validation & save ─────────────────────────────────────────────────────────

bool CustomRowDialog::validate()
{
    // No mandatory fields for generic tables — just ensure the dialog is sane.
    return true;
}

bool CustomRowDialog::save()
{
    if (!m_db || m_def.columns.empty()) return false;

    std::vector<std::string> values;
    for (auto* ctrl : m_fields)
        values.push_back(ctrl->GetValue().ToStdString());

    if (m_rowUuid.empty()) {
        // Insert — generate a UUID4 for the id column
        std::string uuid = dialogGenerateUUID4();
        std::string cols = "id", placeholders = "?";
        for (size_t i = 0; i < m_def.columns.size(); ++i) {
            cols         += ',' + m_def.columns[i].first;
            placeholders += ",?";
        }
        std::string sql = "INSERT INTO user_" + m_tableName +
                          " (" + cols + ") VALUES (" + placeholders + ");";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text(stmt, 1, uuid.c_str(), -1, SQLITE_TRANSIENT);
        for (size_t i = 0; i < values.size(); ++i)
            sqlite3_bind_text(stmt, static_cast<int>(i + 2),
                              values[i].c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    } else {
        // Update
        std::string setClause;
        for (size_t i = 0; i < m_def.columns.size(); ++i) {
            if (i) setClause += ',';
            setClause += m_def.columns[i].first + "=?";
        }
        std::string sql = "UPDATE user_" + m_tableName +
                          " SET " + setClause + " WHERE id=?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return false;
        for (size_t i = 0; i < values.size(); ++i)
            sqlite3_bind_text(stmt, static_cast<int>(i + 1),
                              values[i].c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, static_cast<int>(values.size() + 1),
                          m_rowUuid.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    return true;
}

// ── Event handlers ────────────────────────────────────────────────────────────

void CustomRowDialog::onOK(wxCommandEvent& /*e*/)
{
    if (!validate()) return;
    if (!save()) return;
    EndModal(wxID_OK);
}

void CustomRowDialog::onCancel(wxCommandEvent& /*e*/)
{
    EndModal(wxID_CANCEL);
}
