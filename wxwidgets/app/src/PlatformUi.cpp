#include "PlatformUi.h"

#include "AddTrackDialog.h"

#include <wx/app.h>
#include <wx/menu.h>
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
    for (const auto& item : items)
        menu.Append(item.first, wxString::FromUTF8(item.second));

    int sel = -1;
    menu.Bind(wxEVT_MENU, [&sel](wxCommandEvent& ev) { sel = ev.GetId(); });
    win->PopupMenu(&menu);
    return sel;
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
