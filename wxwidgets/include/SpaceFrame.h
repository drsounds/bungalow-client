#ifndef SPACE_FRAME_H
#define SPACE_FRAME_H
#include "SPWindowElement.h"

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

class SpaceApp;

class SpaceWidget;

namespace space
{
    class SPObject;
    class Space;
    class SPWindowElement;
}

enum {
    ID_CLOSE_WINDOW = wxID_HIGHEST + 1,
    ID_QUIT_APP,

    // File
    ID_FILE_NEW_PLAYLIST,

    // Playback
    ID_PLAYBACK_PLAY_PAUSE,
    ID_PLAYBACK_STOP,
    ID_PLAYBACK_NEXT,
    ID_PLAYBACK_PREV,
    ID_PLAYBACK_SHUFFLE,
    ID_PLAYBACK_REPEAT,
    ID_PLAYBACK_VOL_UP,
    ID_PLAYBACK_VOL_DOWN,

    // Library
    ID_LIBRARY_GO,
    ID_LIBRARY_SCAN,
    ID_LIBRARY_SCAN_FOLDER,

    // Develop
    ID_DEVELOP_SHOW_INSPECTOR,
    ID_DEVELOP_RELOAD_PAGE,
};

class SpaceFrame : public wxFrame
{
protected:
    space::Space *m_space;
    space::SPWindowElement* m_windowElement;
    SpaceWidget *m_spaceWidget;
    wxApp* m_app;
    wxMenu* m_developMenu = nullptr;
	bool m_leftMouseButtonPressed;
public:
    void setSpace(space::Space *space) {
        this->m_space = space;
    }
    space::Space *getSpace() const {
        return this->m_space;
    }
    SpaceWidget *getSpaceWidget() const {
        return this->m_spaceWidget;
    }
    space::SPWindowElement *getWindowElement() const {
        return this->m_windowElement;
    }
    void setWindowElement(space::SPWindowElement *value);
    SpaceFrame(wxApp *app, space::Space *space);
    void OnSize(wxSizeEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnHello(wxCommandEvent& event);
    void OnCloseWindow(wxCommandEvent& event);
    void OnQuitApp(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    // Menu handlers — File
    void OnFileNewPlaylist(wxCommandEvent& event);
    void OnFileOpen(wxCommandEvent& event);

    // Menu handlers — Edit
    void OnEditFind(wxCommandEvent& event);

    // Menu handlers — Playback
    void OnPlaybackPlayPause(wxCommandEvent& event);
    void OnPlaybackStop(wxCommandEvent& event);
    void OnPlaybackNext(wxCommandEvent& event);
    void OnPlaybackPrev(wxCommandEvent& event);
    void OnPlaybackShuffle(wxCommandEvent& event);
    void OnPlaybackRepeat(wxCommandEvent& event);
    void OnPlaybackVolumeUp(wxCommandEvent& event);
    void OnPlaybackVolumeDown(wxCommandEvent& event);

    // Menu handlers — Library
    void OnLibraryGo(wxCommandEvent& event);
    void OnLibraryScan(wxCommandEvent& event);
    void OnLibraryScanFolder(wxCommandEvent& event);

    // Menu handlers — App (macOS Preferences)
    void OnPreferences(wxCommandEvent& event);

    // Menu handlers — Develop
    void OnDevelopShowInspector(wxCommandEvent& event);
    void OnDevelopReloadPage(wxCommandEvent& event);

    /** Add or remove the Develop menu based on developer-mode state. */
    void applyDeveloperMode(bool enabled);

    /** Fire a Space navigation event so the view stack switches to @p uri. */
    void navigateTo(const std::string& uri);

    wxApp *getApp() const { return m_app; }
};

#endif