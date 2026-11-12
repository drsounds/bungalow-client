#include "PlatformUi.h"

#include <Windows.h>
#include <ShlObj.h>
#include <commdlg.h>
#include <shellapi.h>

#include <filesystem>
#include <functional>
#include <regex>

#include "AudioImporter.h"
#include "BungalowApp.h"
#include "Database.h"

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

    // ── Generic labelled-field form, built as an in-memory dialog template ───

    // Returns an error message for invalid input, or "" to accept it.
    using FormValidator = std::function<std::string(const std::vector<std::string>&)>;

    struct FormState {
        std::vector<std::string>* values;
        FormValidator validate;
    };

    constexpr WORD kFirstFieldId = 1000;

    class DialogTemplate {
    public:
        DialogTemplate(const std::wstring& title, short cx, short cy, WORD itemCount) {
            dword(DS_MODALFRAME | DS_SETFONT | DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU);
            dword(0);
            m_buf.push_back(itemCount);
            m_buf.push_back(0); m_buf.push_back(0);
            m_buf.push_back(static_cast<WORD>(cx)); m_buf.push_back(static_cast<WORD>(cy));
            m_buf.push_back(0); // no menu
            m_buf.push_back(0); // default class
            str(title);
            m_buf.push_back(9);
            str(L"Segoe UI");
        }

        void item(WORD classAtom, DWORD style, short x, short y, short cx, short cy, WORD id,
                  const std::wstring& text) {
            if (m_buf.size() % 2) m_buf.push_back(0); // DWORD-align each item
            dword(style | WS_CHILD | WS_VISIBLE);
            dword(0);
            m_buf.push_back(static_cast<WORD>(x)); m_buf.push_back(static_cast<WORD>(y));
            m_buf.push_back(static_cast<WORD>(cx)); m_buf.push_back(static_cast<WORD>(cy));
            m_buf.push_back(id);
            m_buf.push_back(0xFFFF);
            m_buf.push_back(classAtom);
            str(text);
            m_buf.push_back(0); // no creation data
        }

        const DLGTEMPLATE* get() const { return reinterpret_cast<const DLGTEMPLATE*>(m_buf.data()); }

    private:
        std::vector<WORD> m_buf;

        void dword(DWORD d) { m_buf.push_back(LOWORD(d)); m_buf.push_back(HIWORD(d)); }
        void str(const std::wstring& s) {
            m_buf.insert(m_buf.end(), s.begin(), s.end());
            m_buf.push_back(0);
        }
    };

    INT_PTR CALLBACK formDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_INITDIALOG: {
            auto* state = reinterpret_cast<FormState*>(lParam);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, lParam);
            for (size_t i = 0; i < state->values->size(); ++i)
                SetDlgItemTextW(hwnd, kFirstFieldId + static_cast<WORD>(i), widen((*state->values)[i]).c_str());
            return TRUE;
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {
                auto* state = reinterpret_cast<FormState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
                std::vector<std::string> values(state->values->size());
                for (size_t i = 0; i < values.size(); ++i) {
                    HWND field = GetDlgItem(hwnd, kFirstFieldId + static_cast<WORD>(i));
                    std::wstring text(static_cast<size_t>(GetWindowTextLengthW(field)) + 1, L'\0');
                    GetWindowTextW(field, text.data(), static_cast<int>(text.size()));
                    text.resize(wcslen(text.c_str()));
                    values[i] = narrow(text);
                }
                if (state->validate) {
                    const std::string error = state->validate(values);
                    if (!error.empty()) {
                        MessageBoxW(hwnd, widen(error).c_str(), L"Validation", MB_OK | MB_ICONWARNING);
                        return TRUE;
                    }
                }
                *state->values = std::move(values);
                EndDialog(hwnd, IDOK);
                return TRUE;
            }
            if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hwnd, IDCANCEL);
                return TRUE;
            }
            break;
        }
        return FALSE;
    }

    // Shows one labelled edit box per entry in @p labels. @p values supplies the
    // initial text and receives the result; fields flagged in @p readOnly can't
    // be edited. Returns true if the user pressed OK.
    bool runFormDialog(const std::string& title, const std::vector<std::string>& labels,
                       std::vector<std::string>& values, const FormValidator& validate = {},
                       const std::vector<bool>& readOnly = {}) {
        values.resize(labels.size());
        constexpr short kWidth = 260, kMargin = 7, kRowHeight = 26, kButtonWidth = 50;
        const short buttonsY = static_cast<short>(kMargin + kRowHeight * labels.size() + 4);
        const short height   = static_cast<short>(buttonsY + 14 + kMargin);

        DialogTemplate tpl(widen(title), kWidth, height, static_cast<WORD>(labels.size() * 2 + 2));
        for (size_t i = 0; i < labels.size(); ++i) {
            const short y = static_cast<short>(kMargin + kRowHeight * i);
            const DWORD editStyle = (i < readOnly.size() && readOnly[i]) ? ES_READONLY : 0;
            tpl.item(0x0082, SS_LEFT, kMargin, y, kWidth - 2 * kMargin, 9, 0xFFFF, widen(labels[i]));
            tpl.item(0x0081, WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL | editStyle, kMargin, y + 10,
                     kWidth - 2 * kMargin, 12, kFirstFieldId + static_cast<WORD>(i), L"");
        }
        tpl.item(0x0080, BS_DEFPUSHBUTTON | WS_TABSTOP, kWidth - kMargin - 2 * kButtonWidth - 4, buttonsY,
                 kButtonWidth, 14, IDOK, L"OK");
        tpl.item(0x0080, BS_PUSHBUTTON | WS_TABSTOP, kWidth - kMargin - kButtonWidth, buttonsY,
                 kButtonWidth, 14, IDCANCEL, L"Cancel");

        FormState state{ &values, validate };
        return DialogBoxIndirectParamW(GetModuleHandleW(nullptr), tpl.get(), GetActiveWindow(),
                                       formDialogProc, reinterpret_cast<LPARAM>(&state)) == IDOK;
    }

    // ── Single-choice list ───────────────────────────────────────────────────

    constexpr WORD kChoiceListId = 1100;

    struct ChoiceState {
        const std::vector<std::string>* choices;
        int selection = -1;
    };

    INT_PTR CALLBACK choiceDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_INITDIALOG: {
            auto* state = reinterpret_cast<ChoiceState*>(lParam);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, lParam);
            for (const auto& c : *state->choices)
                SendDlgItemMessageW(hwnd, kChoiceListId, LB_ADDSTRING, 0,
                                    reinterpret_cast<LPARAM>(widen(c).c_str()));
            SendDlgItemMessageW(hwnd, kChoiceListId, LB_SETCURSEL, 0, 0);
            return TRUE;
        }
        case WM_COMMAND: {
            const bool doubleClicked = LOWORD(wParam) == kChoiceListId && HIWORD(wParam) == LBN_DBLCLK;
            if (LOWORD(wParam) == IDOK || doubleClicked) {
                auto* state = reinterpret_cast<ChoiceState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
                const LRESULT sel = SendDlgItemMessageW(hwnd, kChoiceListId, LB_GETCURSEL, 0, 0);
                if (sel == LB_ERR) return TRUE;
                state->selection = static_cast<int>(sel);
                EndDialog(hwnd, IDOK);
                return TRUE;
            }
            if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hwnd, IDCANCEL);
                return TRUE;
            }
            break;
        }
        }
        return FALSE;
    }

    bool isIdentifier(const std::string& s) {
        static const std::regex kIdentifier("^[A-Za-z][A-Za-z0-9_]*$");
        return std::regex_match(s, kIdentifier);
    }

    std::string trim(const std::string& s) {
        const auto first = s.find_first_not_of(" \t");
        if (first == std::string::npos) return "";
        return s.substr(first, s.find_last_not_of(" \t") - first + 1);
    }

    // Parses "title:TEXT, year:INTEGER, artist:BELONGS_TO:artists" into column pairs.
    // Returns an error message, or "" on success.
    std::string parseColumns(const std::string& spec, std::vector<std::pair<std::string, std::string>>& cols) {
        cols.clear();
        size_t pos = 0;
        while (pos <= spec.size()) {
            const size_t comma = spec.find(',', pos);
            const std::string entry = trim(spec.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos));
            pos = comma == std::string::npos ? spec.size() + 1 : comma + 1;
            if (entry.empty()) continue;

            const size_t colon = entry.find(':');
            const std::string name = trim(entry.substr(0, colon));
            std::string type = colon == std::string::npos ? "TEXT" : trim(entry.substr(colon + 1));
            if (!isIdentifier(name))
                return "Column name '" + name + "' is not valid.\nUse letters, digits, or underscores (start with a letter).";
            if (type.rfind("BELONGS_TO:", 0) == 0) {
                const std::string target = trim(type.substr(11));
                if (!isIdentifier(target))
                    return "Column '" + name + "' is a relation - enter the target table name.";
                type = "BELONGS_TO:" + target;
            } else if (type != "TEXT" && type != "INTEGER" && type != "REAL") {
                return "Column '" + name + "' has unknown type '" + type + "'.\nUse TEXT, INTEGER, REAL or BELONGS_TO:<table>.";
            }
            cols.emplace_back(name, type);
        }
        return cols.empty() ? "Add at least one column." : "";
    }

    Database* appDatabase() {
        auto* app = BungalowApp::current();
        return app ? app->getDatabase() : nullptr;
    }
}

int showPopupMenu(const MenuItems& items) {
    HWND owner = GetActiveWindow();
    if (!owner) owner = GetForegroundWindow();

    HMENU menu = CreatePopupMenu();
    if (!menu) return -1;
    for (const auto& item : items) {
        if (item.second.empty())
            AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        else
            AppendMenuW(menu, MF_STRING, static_cast<UINT_PTR>(item.first), widen(item.second).c_str());
    }

    POINT pt{};
    GetCursorPos(&pt);
    const BOOL cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, owner, nullptr);
    DestroyMenu(menu);
    return cmd ? static_cast<int>(cmd) : -1;
}

bool showFormDialog(const std::string& title, std::vector<FormField>& fields) {
    std::vector<std::string> labels, values;
    std::vector<bool> readOnly;
    for (const auto& f : fields) {
        // Edit boxes here have no cue banner, so the hint goes in the label.
        std::string label = f.label + (f.required ? " *" : "");
        if (!f.hint.empty()) label += "  (" + f.hint + ")";
        labels.push_back(label);
        values.push_back(f.value);
        readOnly.push_back(f.readOnly);
    }
    const bool ok = runFormDialog(title, labels, values, [&fields](const std::vector<std::string>& v) {
        for (size_t i = 0; i < fields.size(); ++i)
            if (fields[i].required && trim(v[i]).empty())
                return fields[i].label + " is required.";
        return std::string();
    }, readOnly);
    if (!ok) return false;
    for (size_t i = 0; i < fields.size(); ++i)
        fields[i].value = trim(values[i]);
    return true;
}

int showChoiceDialog(const std::string& title, const std::string& message,
                     const std::vector<std::string>& choices) {
    if (choices.empty()) return -1;
    constexpr short kWidth = 260, kMargin = 7, kListHeight = 120, kButtonWidth = 50;
    const short listY    = kMargin + 12;
    const short buttonsY = static_cast<short>(listY + kListHeight + 4);
    const short height   = static_cast<short>(buttonsY + 14 + kMargin);

    DialogTemplate tpl(widen(title), kWidth, height, 4);
    tpl.item(0x0082, SS_LEFT, kMargin, kMargin, kWidth - 2 * kMargin, 9, 0xFFFF, widen(message));
    tpl.item(0x0083, WS_BORDER | WS_VSCROLL | WS_TABSTOP | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
             kMargin, listY, kWidth - 2 * kMargin, kListHeight, kChoiceListId, L"");
    tpl.item(0x0080, BS_DEFPUSHBUTTON | WS_TABSTOP, kWidth - kMargin - 2 * kButtonWidth - 4, buttonsY,
             kButtonWidth, 14, IDOK, L"OK");
    tpl.item(0x0080, BS_PUSHBUTTON | WS_TABSTOP, kWidth - kMargin - kButtonWidth, buttonsY,
             kButtonWidth, 14, IDCANCEL, L"Cancel");

    ChoiceState state{ &choices };
    if (DialogBoxIndirectParamW(GetModuleHandleW(nullptr), tpl.get(), GetActiveWindow(),
                                choiceDialogProc, reinterpret_cast<LPARAM>(&state)) != IDOK)
        return -1;
    return state.selection;
}

// Imports a single audio file chosen by the user into the library, metadata included.
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

    // Reads the embedded tags and breaks the file out into recording, artists,
    // release and track rows, linked to the file and saved to the library.
    return AudioImporter::importFile(db, path).fileId >= 0;
}

bool showCreateTableDialog(sqlite3* db) {
    Database* database = appDatabase();
    if (!db || !database) return false;

    const std::vector<std::string> labels = {
        "Table name (letters, digits, underscores)",
        "Display name",
        "Singular noun (e.g. Artist)",
        "Plural noun (e.g. Artists)",
        "Description",
        "Columns, comma-separated name:TYPE (TEXT, INTEGER, REAL, BELONGS_TO:<table>)",
    };
    std::vector<std::string> values = { "", "", "", "", "", "name:TEXT" };
    std::vector<std::pair<std::string, std::string>> cols;

    const bool ok = runFormDialog("New Table", labels, values, [&](const std::vector<std::string>& v) {
        if (v[0].empty()) return std::string("Table name cannot be empty.");
        if (!isIdentifier(v[0]))
            return std::string("Table name must start with a letter and contain only\nletters, digits, or underscores.");
        if (!database->getCustomTable(v[0]).name.empty())
            return "A table named '" + v[0] + "' already exists.";
        return parseColumns(v[5], cols);
    });
    if (!ok) return false;

    return database->createCustomTable(values[0], cols, values[1], values[2], values[3], values[4]) >= 0;
}

bool showCustomRowDialog(sqlite3* db, const std::string& tableName, const std::string& rowUuid) {
    Database* database = appDatabase();
    if (!db || !database || tableName.empty()) return false;

    const CustomTableDef def = database->getCustomTable(tableName);
    if (def.columns.empty()) return false;

    std::vector<std::string> labels;
    for (const auto& col : def.columns) {
        const std::string& type = col.second;
        labels.push_back(type.rfind("BELONGS_TO:", 0) == 0
            ? col.first + " (link: bungalow:" + type.substr(11) + ":<id>)"
            : col.first + " (" + type + ")");
    }

    std::vector<std::string> values;
    if (!rowUuid.empty()) {
        CustomRow row = database->getCustomRow(tableName, rowUuid);
        if (row.id.empty()) return false;
        values = std::move(row.values);
    }

    const std::string title = (rowUuid.empty() ? "Add " : "Edit ")
        + (def.nounSingular.empty() ? std::string("Row") : def.nounSingular);
    if (!runFormDialog(title, labels, values)) return false;

    if (rowUuid.empty())
        return !database->insertCustomRow(tableName, def, values).empty();
    database->updateCustomRow(tableName, def, rowUuid, values);
    return true;
}

bool showConfirmDialog(const std::string& title, const std::string& message) {
    return MessageBoxW(GetActiveWindow(), widen(message).c_str(), widen(title).c_str(),
                       MB_YESNO | MB_ICONWARNING) == IDYES;
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

void openExternalUrl(const std::string& url) {
    if (url.rfind("https://", 0) != 0 && url.rfind("http://", 0) != 0) return;
    ShellExecuteW(nullptr, L"open", widen(url).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

std::string userDataDir() {
    std::string dir = knownFolder(FOLDERID_RoamingAppData) + "/Bungalow";
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
