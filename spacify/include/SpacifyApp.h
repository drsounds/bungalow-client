#ifndef SPACE_APP_H
#define SPACE_APP_H

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "PlayContext.h"
#include "PlayQueueEntry.h"
#include "Database.h"
#include "SPObservable.h"
#include "DownloadManager.h"

#include <memory>
#include <string>
#include <vector>

class SpaceFrame;
class SPMediaService;
class SPPlayable;

namespace space
{
    class Space;
    class SPWindowElement;
    class MainElement;
}

class SpacifyApp : public wxApp
{
protected:
    space::Space* m_space;
    space::SPWindowElement *m_windowElement;
    SpaceFrame* m_frame;
    space::MainElement* m_main;
    std::unique_ptr<Database> m_database;
    std::string m_databasePath;
    std::unique_ptr<DownloadManager> m_downloadManager;

    /// Registered media service instances, searched in order by acceptsUri.
    std::vector<SPMediaService*> m_mediaServices;

    bool m_developerMode = false;

    // ── Playback state ────────────────────────────────────────────────────────
    /// Non-owning alias into m_queuePlayables[m_queueIndex]; null when stopped.
    SPPlayable*              m_currentPlayable = nullptr;

    /// Display metadata for every slot in the current queue.
    std::vector<PlayQueueEntry> m_queueEntries;
    /// Runtime objects parallel to m_queueEntries; nullptr for unplayable slots.
    std::vector<SPPlayable*>    m_queuePlayables;
    /// Index of the currently-playing entry, or -1 when stopped.
    int                         m_queueIndex = -1;

    bool                     m_isPlaying  = false;
    int                      m_positionMs = 0;
    int                      m_durationMs = 0;

    // ── Now-playing metadata (title, artist, cover art path) ─────────────────
    std::string m_trackTitle;
    std::string m_artistName;
    std::string m_coverArtPath;

    /// Observable that footer (and others) subscribe to for playback events.
    /// Events: "playback:changed", "queue:changed"
    space::SPObservable m_playbackEvents;

    wxTimer m_positionTimer;

    void dispatchPlaybackChanged();
    void onPositionTimer(wxTimerEvent& event);

    /// Stop current playback and timer without modifying queue state.
    void stopCurrentPlayback();

    /// Start playing the entry at idx, skipping forward past unplayable entries.
    void playFromIndex(int idx);

public:
    virtual bool OnInit() override;
    virtual int OnExit() override;
    Database* getDatabase() { return m_database.get(); }
    const std::string& getDatabasePath() const { return m_databasePath; }
    DownloadManager* getDownloadManager() { return m_downloadManager.get(); }

    const std::vector<SPMediaService*>& getMediaServices() const { return m_mediaServices; }
    SPMediaService* getMediaServiceForUri(const std::string& uri) const;

    void navigate(const std::string& uri);

    bool isDeveloperMode() const { return m_developerMode; }
    void setDeveloperMode(bool enabled);
    void setFullWidthAddressBar(bool fullWidth);
    void showWebInspector();
    void reloadWebPage();

    /// Show an error banner in the main view with an optional custom message.
    void showError(const std::string& msg = "An error occurred");

    // ── Playback API ──────────────────────────────────────────────────────────

    /**
     * Primary entry point: populate the queue from ctx.entries and start
     * playing at ctx.startIndex. Fires "queue:changed" then "playback:changed".
     */
    void playContext(PlayContext ctx);

    /**
     * Play the entry at idx within the existing queue without rebuilding it.
     * Does nothing if idx is out of range or the entry is not playable.
     */
    void playQueueAtIndex(int idx);

    /// Compatibility wrappers that build a single-entry PlayContext.
    void playTrackId(int64_t trackId);
    void playUri(const std::string& uri);

    void togglePlayPause();
    void seekTo(int posMs);
    void playbackNext();
    void playbackPrev();

    bool        isPlaying()    const { return m_isPlaying; }
    int         getPositionMs()const { return m_positionMs; }
    int         getDurationMs()const { return m_durationMs; }
    SPPlayable* getCurrentPlayable() const { return m_currentPlayable; }

    const std::vector<PlayQueueEntry>& getQueueEntries() const { return m_queueEntries; }
    int getQueueIndex() const { return m_queueIndex; }

    const std::string& getTrackTitle()   const { return m_trackTitle; }
    const std::string& getArtistName()   const { return m_artistName; }
    const std::string& getCoverArtPath() const { return m_coverArtPath; }

    space::SPObservable& getPlaybackEvents() { return m_playbackEvents; }

    wxDECLARE_EVENT_TABLE();
};

#endif
