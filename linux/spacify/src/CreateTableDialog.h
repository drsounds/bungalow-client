#pragma once

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/scrolwin.h>

#include <sqlite3.h>
#include <string>
#include <vector>

#include "Database.h"

/**
 * CreateTableDialog — modal dialog for defining a new custom database table.
 *
 * Collects:
 *   • Table name (db-safe identifier)
 *   • Display name, noun (singular/plural), description — stored in db_tables
 *   • Columns: name + type (TEXT / INTEGER / REAL / BELONGS_TO:<target>)
 *
 * On wxID_OK the table is created in the database before the dialog closes.
 */
class CreateTableDialog : public wxDialog {
public:
    explicit CreateTableDialog(wxWindow* parent, sqlite3* db);
    ~CreateTableDialog() override = default;

    /** Returns the name of the newly created table (valid only after wxID_OK). */
    const std::string& getCreatedTableName() const { return m_createdName; }

private:
    // ── State ─────────────────────────────────────────────────────────────────
    sqlite3*    m_db = nullptr;
    std::string m_createdName;

    // ── UI — metadata fields ──────────────────────────────────────────────────
    wxTextCtrl* m_nameCtrl         = nullptr;
    wxTextCtrl* m_displayNameCtrl  = nullptr;
    wxTextCtrl* m_nounSingularCtrl = nullptr;
    wxTextCtrl* m_nounPluralCtrl   = nullptr;
    wxTextCtrl* m_descriptionCtrl  = nullptr;

    // ── UI — column rows ──────────────────────────────────────────────────────
    wxScrolledWindow* m_colsPanel = nullptr;
    wxBoxSizer*       m_colsSizer = nullptr;

    struct ColRow {
        wxPanel*    panel      = nullptr;
        wxTextCtrl* nameCtrl   = nullptr;
        wxChoice*   typeCtrl   = nullptr;
        wxTextCtrl* targetCtrl = nullptr; // visible only for BELONGS_TO
        wxButton*   removeBtn  = nullptr;
    };
    std::vector<ColRow> m_colRows;

    // ── Helpers ───────────────────────────────────────────────────────────────
    void buildUI();
    void addColRow(const std::string& name = "", const std::string& type = "TEXT");
    void removeColRow(wxButton* btn);
    void refreshColsPanel();

    bool validate();
    bool save();

    // ── Event handlers ────────────────────────────────────────────────────────
    void onAddCol(wxCommandEvent& e);
    void onRemoveCol(wxCommandEvent& e);
    void onOK(wxCommandEvent& e);
    void onCancel(wxCommandEvent& e);
};
