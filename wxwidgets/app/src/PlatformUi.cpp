#include "PlatformUi.h"

#include "AddTrackDialog.h"
#include "CreateTableDialog.h"
#include "CustomRowDialog.h"

#include <wx/app.h>
#include <wx/choicdlg.h>
#include <wx/dialog.h>
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/msgdlg.h>
#include <wx/dirdlg.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>

namespace space {
namespace platform {

int showPopupMenu(const MenuItems& items) {
    wxWindow* win = wxTheApp ? wxTheApp->GetTopWindow() : nullptr;
    if (!win) return -1;

    wxMenu menu;
    for (const auto& item : items) {
        if (item.second.empty())
            menu.AppendSeparator();
        else
            menu.Append(item.first, wxString::FromUTF8(item.second));
    }

    int sel = -1;
    menu.Bind(wxEVT_MENU, [&sel](wxCommandEvent& ev) { sel = ev.GetId(); });
    win->PopupMenu(&menu);
    return sel;
}

static std::string trimmed(const wxString& s) {
    wxString v = s;
    v.Trim(true).Trim(false);
    return std::string(v.ToUTF8().data());
}

bool showFormDialog(const std::string& title, std::vector<FormField>& fields) {
    wxWindow* parent = wxTheApp ? wxTheApp->GetTopWindow() : nullptr;
    wxDialog dlg(parent, wxID_ANY, wxString::FromUTF8(title), wxDefaultPosition, wxDefaultSize,
                 wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);

    auto* root = new wxBoxSizer(wxVERTICAL);
    auto* grid = new wxFlexGridSizer(2, 8, 8);
    grid->AddGrowableCol(1, 1);
    std::vector<wxTextCtrl*> ctrls;
    for (const auto& f : fields) {
        grid->Add(new wxStaticText(&dlg, wxID_ANY,
                      wxString::FromUTF8(f.label + (f.required ? " *" : "") + ":")),
                  0, wxALIGN_CENTER_VERTICAL);
        auto* ctrl = new wxTextCtrl(&dlg, wxID_ANY, wxString::FromUTF8(f.value),
                                    wxDefaultPosition, wxSize(340, -1));
        if (!f.hint.empty()) ctrl->SetHint(wxString::FromUTF8(f.hint));
        if (f.readOnly) ctrl->SetEditable(false);
        grid->Add(ctrl, 1, wxEXPAND);
        ctrls.push_back(ctrl);
    }
    root->Add(grid, 1, wxEXPAND | wxALL, 12);
    root->Add(dlg.CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 12);
    dlg.SetSizerAndFit(root);
    dlg.CentreOnParent();

    dlg.Bind(wxEVT_BUTTON, [&](wxCommandEvent& e) {
        for (size_t i = 0; i < fields.size(); ++i) {
            if (fields[i].required && trimmed(ctrls[i]->GetValue()).empty()) {
                wxMessageBox(wxString::FromUTF8(fields[i].label + " is required."),
                             "Missing value", wxOK | wxICON_WARNING, &dlg);
                ctrls[i]->SetFocus();
                return;
            }
        }
        e.Skip(); // let the default handler close the dialog with wxID_OK
    }, wxID_OK);

    if (dlg.ShowModal() != wxID_OK) return false;
    for (size_t i = 0; i < fields.size(); ++i)
        fields[i].value = trimmed(ctrls[i]->GetValue());
    return true;
}

int showChoiceDialog(const std::string& title, const std::string& message,
                     const std::vector<std::string>& choices) {
    wxWindow* parent = wxTheApp ? wxTheApp->GetTopWindow() : nullptr;
    wxArrayString items;
    for (const auto& c : choices) items.Add(wxString::FromUTF8(c));
    wxSingleChoiceDialog dlg(parent, wxString::FromUTF8(message), wxString::FromUTF8(title), items);
    return dlg.ShowModal() == wxID_OK ? dlg.GetSelection() : -1;
}

bool showAddTrackDialog(sqlite3* db, int trackId) {
    wxWindow* parent = wxTheApp ? wxTheApp->GetTopWindow() : nullptr;
    if (!parent || !db) return false;

    if (trackId > 0) {
        AddTrackDialog dlg(parent, db, trackId);
        return dlg.ShowModal() == wxID_OK;
    }
    AddTrackDialog dlg(parent, db);
    return dlg.ShowModal() == wxID_OK;
}

bool showCreateTableDialog(sqlite3* db) {
    wxWindow* parent = wxTheApp ? wxTheApp->GetTopWindow() : nullptr;
    if (!parent || !db) return false;

    CreateTableDialog dlg(parent, db);
    return dlg.ShowModal() == wxID_OK;
}

bool showCustomRowDialog(sqlite3* db, const std::string& tableName, const std::string& rowUuid) {
    wxWindow* parent = wxTheApp ? wxTheApp->GetTopWindow() : nullptr;
    if (!parent || !db || tableName.empty()) return false;

    if (!rowUuid.empty()) {
        CustomRowDialog dlg(parent, db, tableName, rowUuid);
        return dlg.ShowModal() == wxID_OK;
    }
    CustomRowDialog dlg(parent, db, tableName);
    return dlg.ShowModal() == wxID_OK;
}

bool showConfirmDialog(const std::string& title, const std::string& message) {
    wxWindow* parent = wxTheApp ? wxTheApp->GetTopWindow() : nullptr;
    return wxMessageBox(wxString::FromUTF8(message), wxString::FromUTF8(title),
                        wxYES_NO | wxICON_WARNING, parent) == wxYES;
}

std::string showFolderDialog(const std::string& title, const std::string& initialPath) {
    wxWindow* parent = wxTheApp ? wxTheApp->GetTopWindow() : nullptr;
    wxDirDialog dlg(parent, wxString::FromUTF8(title), wxString::FromUTF8(initialPath), wxDD_DEFAULT_STYLE);
    if (dlg.ShowModal() != wxID_OK) return "";
    return dlg.GetPath().ToStdString();
}

std::string userDataDir() {
    wxFileName path;
    path.AssignDir(wxStandardPaths::Get().GetUserDataDir());
    path.Normalize(wxPATH_NORM_ALL);
    wxFileName::Mkdir(path.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    return path.GetPath().ToStdString();
}

std::string homeDir() {
    return wxGetHomeDir().ToStdString();
}

std::string documentsDir() {
    return wxStandardPaths::Get().GetDocumentsDir().ToStdString();
}

} // namespace platform
} // namespace space
