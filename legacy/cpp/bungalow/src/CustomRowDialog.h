#pragma once

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <sqlite3.h>
#include <string>
#include <vector>

#include "Database.h"

/**
 * CustomRowDialog — modal dialog for adding or editing a row in a user-defined table.
 *
 * Add mode:  CustomRowDialog(parent, db, tableName)
 * Edit mode: CustomRowDialog(parent, db, tableName, rowId)
 *
 * On wxID_OK the row is inserted/updated in the database.
 * The caller should refresh the table view after ShowModal() == wxID_OK.
 */
class CustomRowDialog : public wxDialog {
public:
    /** Add mode — inserts a new row on OK. */
    CustomRowDialog(wxWindow* parent, sqlite3* db, const std::string& tableName);

    /** Edit mode — pre-fills existing values and updates row on OK. */
    CustomRowDialog(wxWindow* parent, sqlite3* db,
                    const std::string& tableName, const std::string& rowUuid);

    ~CustomRowDialog() override = default;

private:
    sqlite3*     m_db;
    std::string  m_tableName;
    std::string  m_rowUuid;   // empty = add mode

    CustomTableDef           m_def;
    std::vector<wxTextCtrl*> m_fields; // one per column, in order

    void buildUI();
    void populateForEdit();

    bool validate();
    bool save();

    void onOK(wxCommandEvent& e);
    void onCancel(wxCommandEvent& e);
};
