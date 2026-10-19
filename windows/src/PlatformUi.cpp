#include "PlatformUi.h"

#include <Windows.h>
#include <ShlObj.h>
#include <commdlg.h>

#include <filesystem>

namespace space {
namespace platform {

namespace {
    std::wstring widen(const std::string& s) {
        if (s.empty()) return {};
        int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
        std::wstring w(n > 0 ? n - 1 : 0, L'\0');
        if (n > 0) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), n);
        return w;
    }

    std::string narrow(const std::wstring& w) {
        if (w.empty()) return {};
        int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string s(n > 0 ? n - 1 : 0, '\0');
        if (n > 0) WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, s.data(), n, nullptr, nullptr);
        return s;
    }

    std::string knownFolder(REFKNOWNFOLDERID id) {
        PWSTR raw = nullptr;
        std::string result;
        if (SUCCEEDED(SHGetKnownFolderPath(id, 0, nullptr, &raw)) && raw) {
            result = narrow(raw);
            CoTaskMemFree(raw);
        }
        for (char& c : result) if (c == '\\') c = '/';
        return result;
    }

    bool runStatement(sqlite3* db, const char* sql, const std::string& a, int64_t* idOut = nullptr) {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text(stmt, 1, a.c_str(), -1, SQLITE_TRANSIENT);
        const int rc = sqlite3_step(stmt);
        if (idOut && rc == SQLITE_ROW) *idOut = sqlite3_column_int64(stmt, 0);
        sqlite3_finalize(stmt);
        return rc == SQLITE_ROW || rc == SQLITE_DONE;
    }
}

int showPopupMenu(const MenuItems& items) {
    HWND owner = GetActiveWindow();
    if (!owner) owner = GetForegroundWindow();

    HMENU menu = CreatePopupMenu();
    if (!menu) return -1;
    for (const auto& item : items)
        AppendMenuW(menu, MF_STRING, static_cast<UINT_PTR>(item.first), widen(item.second).c_str());

    POINT pt{};
    GetCursorPos(&pt);
    const BOOL cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, owner, nullptr);
    DestroyMenu(menu);
    return cmd ? static_cast<int>(cmd) : -1;
}

// Imports a single audio file chosen by the user into the library.
bool showAddTrackDialog(sqlite3* db, int trackId) {
    if (!db) return false;
    HWND owner = GetActiveWindow();

    if (trackId > 0) {
        MessageBoxW(owner, L"Editing track metadata is not available in the Windows build yet.",
                    L"Edit Track", MB_OK | MB_ICONINFORMATION);
        return false;
    }

    wchar_t file[MAX_PATH * 4] = L"";
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = L"Audio files\0*.mp3;*.m4a;*.aac;*.wav;*.flac;*.ogg;*.opus;*.wma\0All files\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = static_cast<DWORD>(sizeof(file) / sizeof(file[0]));
    ofn.lpstrTitle = L"Add Track";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
    if (!GetOpenFileNameW(&ofn)) return false;

    std::string path = narrow(file);
    for (char& c : path) if (c == '\\') c = '/';
    const std::string name = std::filesystem::path(file).stem().string();

    int64_t fileId = 0;
    runStatement(db, "INSERT OR IGNORE INTO files(path) VALUES(?)", path);
    runStatement(db, "SELECT id FROM files WHERE path=?", path, &fileId);
    if (fileId == 0) return false;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "INSERT OR IGNORE INTO audio_files(file_id, name) VALUES(?,?)",
                           -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, fileId);
        sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    runStatement(db, "INSERT OR IGNORE INTO playables(uri) VALUES(?)", "file://" + path);
    return true;
}

std::string showFolderDialog(const std::string& title, const std::string& initialPath) {
    IFileDialog* dlg = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dlg))))
        return "";

    std::string result;
    DWORD options = 0;
    dlg->GetOptions(&options);
    dlg->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
    dlg->SetTitle(widen(title).c_str());

    if (!initialPath.empty()) {
        IShellItem* folder = nullptr;
        std::wstring wp = widen(initialPath);
        for (auto& c : wp) if (c == L'/') c = L'\\';
        if (SUCCEEDED(SHCreateItemFromParsingName(wp.c_str(), nullptr, IID_PPV_ARGS(&folder))) && folder) {
            dlg->SetFolder(folder);
            folder->Release();
        }
    }

    if (SUCCEEDED(dlg->Show(GetActiveWindow()))) {
        IShellItem* item = nullptr;
        if (SUCCEEDED(dlg->GetResult(&item)) && item) {
            PWSTR raw = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &raw)) && raw) {
                result = narrow(raw);
                CoTaskMemFree(raw);
            }
            item->Release();
        }
    }
    dlg->Release();
    for (char& c : result) if (c == '\\') c = '/';
    return result;
}

std::string userDataDir() {
    std::string dir = knownFolder(FOLDERID_RoamingAppData) + "/Spacify";
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::u8path(dir), ec);
    return dir;
}

std::string homeDir() {
    return knownFolder(FOLDERID_Profile);
}

std::string documentsDir() {
    return knownFolder(FOLDERID_Documents);
}

} // namespace platform
} // namespace space
