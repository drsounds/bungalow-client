#include "SpaceFrame.h"
#include "SpacifyApp.h"
#include "MusicScanner.h"

#include "SpaceWidget.h"
#include "SPObject.h"
#include "Space.h"
#include "SPWindowElement.h"

#include <wx/filedlg.h>
#include <wx/dirdlg.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <stdlib.h>

// ── Helpers ───────────────────────────────────────────────────────────────────

static SpacifyApp* spacifyApp() {
    return dynamic_cast<SpacifyApp*>(wxTheApp);
}

// ── Menu bar construction ─────────────────────────────────────────────────────

static wxMenuBar* buildMenuBar() {
    wxMenuBar* mb = new wxMenuBar();

    // ── File ──────────────────────────────────────────────────────────────────
    wxMenu* fileMenu = new wxMenu();
    fileMenu->Append(ID_FILE_NEW_PLAYLIST, "&New Playlist\tCtrl+N",
                     "Create a new empty playlist");
    fileMenu->Append(wxID_OPEN, "&Open\u2026\tCtrl+O",
                     "Open a local audio file");
    fileMenu->AppendSeparator();
    fileMenu->Append(ID_CLOSE_WINDOW, "&Close Window\tCtrl+W",
                     "Close the current window");
#ifndef __WXMAC__
    // On macOS Quit lives in the app-name menu; suppress it here.
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT, "&Quit\tCtrl+Q", "Quit Spacify");
#endif
    mb->Append(fileMenu, "&File");

    // ── Edit ──────────────────────────────────────────────────────────────────
    wxMenu* editMenu = new wxMenu();
    editMenu->Append(wxID_UNDO, "&Undo\tCtrl+Z");
    editMenu->Append(wxID_REDO, "&Redo\tCtrl+Shift+Z");
    editMenu->AppendSeparator();
    editMenu->Append(wxID_CUT,       "Cu&t\tCtrl+X");
    editMenu->Append(wxID_COPY,      "&Copy\tCtrl+C");
    editMenu->Append(wxID_PASTE,     "&Paste\tCtrl+V");
    editMenu->AppendSeparator();
    editMenu->Append(wxID_SELECTALL, "Select &All\tCtrl+A");
    editMenu->Append(wxID_FIND,      "&Find\u2026\tCtrl+F");
    mb->Append(editMenu, "&Edit");

    // ── Playback ──────────────────────────────────────────────────────────────
    wxMenu* playbackMenu = new wxMenu();
    playbackMenu->Append(ID_PLAYBACK_PLAY_PAUSE, "&Play / Pause\tSpace",
                         "Toggle playback");
    playbackMenu->Append(ID_PLAYBACK_STOP,       "S&top\tCtrl+.",
                         "Stop playback");
    playbackMenu->AppendSeparator();
    playbackMenu->Append(ID_PLAYBACK_NEXT, "&Next\tCtrl+Right",
                         "Skip to next track");
    playbackMenu->Append(ID_PLAYBACK_PREV, "Pre&vious\tCtrl+Left",
                         "Skip to previous track");
    playbackMenu->AppendSeparator();
    playbackMenu->AppendCheckItem(ID_PLAYBACK_SHUFFLE, "S&huffle",
                                  "Toggle shuffle mode");
    playbackMenu->AppendCheckItem(ID_PLAYBACK_REPEAT,  "&Repeat",
                                  "Toggle repeat mode");
    playbackMenu->AppendSeparator();
    playbackMenu->Append(ID_PLAYBACK_VOL_UP,   "Volume &Up\tCtrl+Up",
                         "Increase volume");
    playbackMenu->Append(ID_PLAYBACK_VOL_DOWN, "Volume &Down\tCtrl+Down",
                         "Decrease volume");
    mb->Append(playbackMenu, "&Playback");

    // ── Library ───────────────────────────────────────────────────────────────
    wxMenu* libraryMenu = new wxMenu();
    libraryMenu->Append(ID_LIBRARY_GO, "&Go to Library\tCtrl+L",
                        "Show the Library view");
    libraryMenu->AppendSeparator();
    libraryMenu->Append(ID_LIBRARY_SCAN,        "&Scan Files\u2026",
                        "Scan files and import metadata");
    libraryMenu->Append(ID_LIBRARY_SCAN_FOLDER, "Scan &Music Folder\u2026",
                        "Choose a folder and scan all audio files inside");
    mb->Append(libraryMenu, "&Library");

    // ── macOS app-name menu additions ─────────────────────────────────────────
    // wxID_PREFERENCES is automatically moved to the application menu on macOS.
    // Placing it in any menu here satisfies wxWidgets; the OS promotes it.
#ifdef __WXMAC__
    wxMenu* appMenu = new wxMenu();
    appMenu->Append(wxID_PREFERENCES, "&Settings\u2026\tCtrl+,",
                    "Open application settings");
    mb->Append(appMenu, "Spacify");
#endif

    return mb;
}

// ── Constructor ───────────────────────────────────────────────────────────────

SpaceFrame::SpaceFrame(wxApp *app, space::Space *space)
    : wxFrame(nullptr, wxID_ANY, "Spacify", wxDefaultPosition, wxSize(1280, 720), wxDEFAULT_FRAME_STYLE)
{
    this->m_app = app;
    this->m_space = space;

#ifdef __WXMAC__
    // Hide the title bar header while keeping the traffic-light buttons.
    // Implemented in SpaceFrameMac.mm (Objective-C++).
    extern void ConfigureMacTitlebar(void*);
    ConfigureMacTitlebar(MacGetTopLevelWindowRef());
#endif

    SetMenuBar(buildMenuBar());

    this->Bind(wxEVT_SIZE,         &SpaceFrame::OnSize,  this);
    this->Bind(wxEVT_CLOSE_WINDOW, &SpaceFrame::OnClose, this);

    // Accelerator-only shortcuts kept as fallback on non-menu platforms
    wxAcceleratorEntry entries[2];
    entries[0].Set(wxACCEL_CMD, 'W', ID_CLOSE_WINDOW);
    entries[1].Set(wxACCEL_CMD, 'Q', ID_QUIT_APP);
    this->SetAcceleratorTable(wxAcceleratorTable(2, entries));

    // File
    Bind(wxEVT_MENU, &SpaceFrame::OnFileNewPlaylist, this, ID_FILE_NEW_PLAYLIST);
    Bind(wxEVT_MENU, &SpaceFrame::OnFileOpen,        this, wxID_OPEN);
    Bind(wxEVT_MENU, &SpaceFrame::OnCloseWindow,     this, ID_CLOSE_WINDOW);
    Bind(wxEVT_MENU, &SpaceFrame::OnQuitApp,         this, ID_QUIT_APP);
    Bind(wxEVT_MENU, &SpaceFrame::OnQuitApp,         this, wxID_EXIT);

    // Edit
    Bind(wxEVT_MENU, &SpaceFrame::OnEditFind, this, wxID_FIND);

    // Playback
    Bind(wxEVT_MENU, &SpaceFrame::OnPlaybackPlayPause,  this, ID_PLAYBACK_PLAY_PAUSE);
    Bind(wxEVT_MENU, &SpaceFrame::OnPlaybackStop,       this, ID_PLAYBACK_STOP);
    Bind(wxEVT_MENU, &SpaceFrame::OnPlaybackNext,       this, ID_PLAYBACK_NEXT);
    Bind(wxEVT_MENU, &SpaceFrame::OnPlaybackPrev,       this, ID_PLAYBACK_PREV);
    Bind(wxEVT_MENU, &SpaceFrame::OnPlaybackShuffle,    this, ID_PLAYBACK_SHUFFLE);
    Bind(wxEVT_MENU, &SpaceFrame::OnPlaybackRepeat,     this, ID_PLAYBACK_REPEAT);
    Bind(wxEVT_MENU, &SpaceFrame::OnPlaybackVolumeUp,   this, ID_PLAYBACK_VOL_UP);
    Bind(wxEVT_MENU, &SpaceFrame::OnPlaybackVolumeDown, this, ID_PLAYBACK_VOL_DOWN);

    // Library
    Bind(wxEVT_MENU, &SpaceFrame::OnLibraryGo,        this, ID_LIBRARY_GO);
    Bind(wxEVT_MENU, &SpaceFrame::OnLibraryScan,       this, ID_LIBRARY_SCAN);
    Bind(wxEVT_MENU, &SpaceFrame::OnLibraryScanFolder, this, ID_LIBRARY_SCAN_FOLDER);

    // App menu / Preferences
    Bind(wxEVT_MENU, &SpaceFrame::OnPreferences, this, wxID_PREFERENCES);

    this->m_spaceWidget = new SpaceWidget(dynamic_cast<wxWindow *>(this), wxID_ANY, "Hello World");
    this->m_spaceWidget->setSpace(this->m_space);
}

// ── Navigation helper ─────────────────────────────────────────────────────────

void SpaceFrame::navigateTo(const std::string& uri) {
    if (auto* a = spacifyApp()) a->navigate(uri);
}

// ── Window element ────────────────────────────────────────────────────────────

void SpaceFrame::setWindowElement(space::SPWindowElement* value)
{
    this->m_windowElement = value;
    this->m_windowElement->setAbsoluteBounds({ 0, 0,
        this->getSpaceWidget()->GetSize().GetWidth(),
        this->getSpaceWidget()->GetSize().GetHeight() });
}

// ── Size / close ──────────────────────────────────────────────────────────────

void SpaceFrame::OnSize(wxSizeEvent& event) {
    wxSize clientSize = GetClientSize();
    this->m_spaceWidget->SetSize(clientSize);
    wxSizeEvent widgetEvent(clientSize);
    this->m_spaceWidget->OnSize(widgetEvent);
}

void SpaceFrame::OnClose(wxCloseEvent& event) { event.Skip(); }
void SpaceFrame::OnExit(wxCommandEvent&)       { Close(true); }
void SpaceFrame::OnAbout(wxCommandEvent&)      {}
void SpaceFrame::OnHello(wxCommandEvent&)      {}
void SpaceFrame::OnCloseWindow(wxCommandEvent&){ Close(true); }
void SpaceFrame::OnQuitApp(wxCommandEvent&)    { Close(true); }

// ── File ──────────────────────────────────────────────────────────────────────

void SpaceFrame::OnFileNewPlaylist(wxCommandEvent&) {
    navigateTo("spacify:library:newplaylist");
}

void SpaceFrame::OnFileOpen(wxCommandEvent&) {
    wxFileDialog dlg(this, "Open audio file", "", "",
                     "Audio files (*.mp3;*.flac;*.aac;*.ogg;*.wav)"
                     "|*.mp3;*.flac;*.aac;*.ogg;*.wav|All files (*.*)|*.*",
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() == wxID_OK)
        navigateTo("spacify:file:" + dlg.GetPath().ToStdString());
}

// ── Edit ──────────────────────────────────────────────────────────────────────

void SpaceFrame::OnEditFind(wxCommandEvent&) {
    navigateTo("spacify:search");
}

// ── Playback ──────────────────────────────────────────────────────────────────

void SpaceFrame::OnPlaybackPlayPause(wxCommandEvent&)  { navigateTo("spacify:playback:playpause"); }
void SpaceFrame::OnPlaybackStop(wxCommandEvent&)       { navigateTo("spacify:playback:stop"); }
void SpaceFrame::OnPlaybackNext(wxCommandEvent&)       { navigateTo("spacify:playback:next"); }
void SpaceFrame::OnPlaybackPrev(wxCommandEvent&)       { navigateTo("spacify:playback:prev"); }
void SpaceFrame::OnPlaybackVolumeUp(wxCommandEvent&)   { navigateTo("spacify:playback:volume:up"); }
void SpaceFrame::OnPlaybackVolumeDown(wxCommandEvent&) { navigateTo("spacify:playback:volume:down"); }

void SpaceFrame::OnPlaybackShuffle(wxCommandEvent& e) {
    navigateTo(e.IsChecked() ? "spacify:playback:shuffle:on"
                              : "spacify:playback:shuffle:off");
}
void SpaceFrame::OnPlaybackRepeat(wxCommandEvent& e) {
    navigateTo(e.IsChecked() ? "spacify:playback:repeat:on"
                              : "spacify:playback:repeat:off");
}

// ── Library ───────────────────────────────────────────────────────────────────

void SpaceFrame::OnLibraryGo(wxCommandEvent&) { navigateTo("spacify:library"); }

void SpaceFrame::OnLibraryScan(wxCommandEvent&) {
    wxFileDialog dlg(this, "Select audio files to scan", "", "",
                     "Audio files (*.mp3;*.flac;*.aac;*.ogg;*.wav)"
                     "|*.mp3;*.flac;*.aac;*.ogg;*.wav|All files (*.*)|*.*",
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
    if (dlg.ShowModal() == wxID_OK)
        navigateTo("spacify:library:scan");
}

void SpaceFrame::OnLibraryScanFolder(wxCommandEvent&) {
    wxDirDialog dirDlg(this, "Select music folder to scan", "",
                       wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
    if (dirDlg.ShowModal() != wxID_OK) return;

    std::string folderPath = dirDlg.GetPath().ToStdString();

    // Resolve database path the same way SpacifyApp does
    std::string dbPath;
    if (auto* app = dynamic_cast<SpacifyApp*>(wxTheApp))
        dbPath = app->getDatabasePath();

    if (dbPath.empty()) {
        // Fallback: compute from executable location
        wxFileName exe(wxStandardPaths::Get().GetExecutablePath());
        wxFileName appDir;
        appDir.AssignDir(exe.GetPath() + "/../resources/app");
        appDir.Normalize(wxPATH_NORM_ALL);
        dbPath = appDir.GetPath().ToStdString() + "/../spacify.db";
    }

    MusicScanDialog scanDlg(this, folderPath, dbPath);
    scanDlg.ShowModal();
}

// ── App menu / Preferences ────────────────────────────────────────────────────

void SpaceFrame::OnPreferences(wxCommandEvent&) {
    navigateTo("spacify:config");
}

// ── Develop menu ──────────────────────────────────────────────────────────────

void SpaceFrame::applyDeveloperMode(bool enabled) {
    wxMenuBar* mb = GetMenuBar();
    if (!mb) return;

    if (enabled && !m_developMenu) {
        m_developMenu = new wxMenu();
        m_developMenu->Append(ID_DEVELOP_SHOW_INSPECTOR, "Show &Inspector",
                              "Open the Web Inspector for the active web view");
        m_developMenu->Append(ID_DEVELOP_RELOAD_PAGE, "&Reload Page\tCtrl+R",
                              "Reload the current web page");
        mb->Append(m_developMenu, "&Develop");
        Bind(wxEVT_MENU, &SpaceFrame::OnDevelopShowInspector, this, ID_DEVELOP_SHOW_INSPECTOR);
        Bind(wxEVT_MENU, &SpaceFrame::OnDevelopReloadPage,    this, ID_DEVELOP_RELOAD_PAGE);
    } else if (!enabled && m_developMenu) {
        Unbind(wxEVT_MENU, &SpaceFrame::OnDevelopShowInspector, this, ID_DEVELOP_SHOW_INSPECTOR);
        Unbind(wxEVT_MENU, &SpaceFrame::OnDevelopReloadPage,    this, ID_DEVELOP_RELOAD_PAGE);
        int pos = mb->FindMenu("Develop");
        if (pos != wxNOT_FOUND)
            delete mb->Remove(pos);
        m_developMenu = nullptr;
    }
}

void SpaceFrame::OnDevelopShowInspector(wxCommandEvent&) {
    if (auto* a = spacifyApp()) a->showWebInspector();
}

void SpaceFrame::OnDevelopReloadPage(wxCommandEvent&) {
    if (auto* a = spacifyApp()) a->reloadWebPage();
}
