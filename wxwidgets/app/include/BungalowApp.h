#ifndef SPACE_APP_H
#define SPACE_APP_H

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "PlayContext.h"
#include "PlayQueueEntry.h"
#include "Database.h"
#include "SPObservable.h"
#include "SPApplicationContext.h"
#include "DownloadManager.h"
#include "service/AvailabilityResolver.h"
#include "service/BackgroundWorker.h"
#include "service/PlaybackCoordinator.h"
#include "service/ResolutionStore.h"
#include "service/ServiceManager.h"
#include "plugin/PluginManager.h"
#include "plugin/PythonHost.h"

#include <memory>
#include <string>
#include <vector>

class SpaceFrame;

namespace space
{
    class Space;
    class SPWindowElement;
    namespace ui { class MainWindow; }
}

class BungalowApp : public wxApp, public space::SPApplicationContext
{
protected:
    space::Space* m_space;
    space::SPWindowElement *m_windowElement;
    SpaceFrame* m_frame;
    space::ui::MainWindow* m_main;
    std::unique_ptr<Database> m_database;
    std::string m_databasePath;
    std::unique_ptr<DownloadManager> m_downloadManager;

    /// Installed services, selected by capability. Declared after m_database
    /// so they are destroyed before it.
    space::service::ServiceManager                       m_services;
    std::unique_ptr<space::service::ResolutionStore>     m_resolutionStore;
    /// Resolves each queue entry to a service at play time.
    std::unique_ptr<space::service::PlaybackCoordinator> m_coordinator;
    /// Which services can play a track; views hold weak references.
    std::shared_ptr<space::service::AvailabilityResolver> m_availability;
    /// Where service lookups run, one at a time.
    std::unique_ptr<space::service::BackgroundWorker>    m_worker;
    // The embedded interpreter for plugins written in Python; null when this
    // build has none. Declared before m_plugins: it must outlive their objects.
    std::unique_ptr<space::plugin::PythonHost>           m_python;
    // Externally loaded plugins (<app>/Apps, ~/Bungalow) and the services they
    // registered. Declared after m_services, so it is destroyed before it.
    std::unique_ptr<space::plugin::PluginManager>        m_plugins;

    bool m_developerMode = false;

    // ── Playback state ────────────────────────────────────────────────────────
    /// A queue entry is loaded on a service; false when stopped.
    bool                        m_hasCurrent = false;

    /// Display metadata for every slot in the current queue.
    std::vector<PlayQueueEntry> m_queueEntries;
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
    /// chooseService asks which service plays it ("Play with…").
    void playFromIndex(int idx, bool chooseService = false);

    /// Finishes playFromIndex() once the entry is resolved to a service.
    void onResolved(int idx, const space::service::PlaybackCoordinator::Result& result);

    /// Whether some service could play the entry: its own URI, or its ISRC.
    bool canResolve(const PlayQueueEntry& entry) const;

public:
    static BungalowApp* current() { return dynamic_cast<BungalowApp*>(wxTheApp); }
    virtual bool OnInit() override;
    virtual int OnExit() override;
    Database* getDatabase() { return m_database.get(); }
    const std::string& getDatabasePath() const { return m_databasePath; }
    DownloadManager* getDownloadManager() { return m_downloadManager.get(); }

    /// Installed services; select them by capability with getServices<T>().
    space::service::ServiceManager& getServiceManager() { return m_services; }
    // Null before the services are set up.
    space::plugin::PluginManager* getPluginManager() { return m_plugins.get(); }
    /// Null before OnInit().
    space::service::PlaybackCoordinator* getPlaybackCoordinator() { return m_coordinator.get(); }
    /// The service that is playing (or last played), or null.
    space::service::MediaService* getActiveService() const;
    /// Which services can play a track, for greying out track lists. Empty
    /// before OnInit() and after OnExit().
    std::weak_ptr<space::service::AvailabilityResolver> getAvailabilityResolver() const { return m_availability; }

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
    // Plays a library track (bungalow:track:<id>), a recording
    // (musik:isrc:<ISRC>) or a URI a media service accepts.
    void playUri(const std::string& uri) override;
    bool canPlayUri(const std::string& uri) const override;

    void togglePlayPause() override;
    void seekTo(int posMs) override;
    void playbackNext() override;
    void playbackPrev() override;

    bool        isPlaying()    const { return m_isPlaying; }
    int         getPositionMs()const { return m_positionMs; }
    int         getDurationMs()const { return m_durationMs; }
    bool hasCurrentPlayable() const override { return m_hasCurrent; }
    std::string currentPlayableUri() const override;

    const std::vector<PlayQueueEntry>& getQueueEntries() const { return m_queueEntries; }
    int getQueueIndex() const { return m_queueIndex; }

    const std::string& getTrackTitle()   const override { return m_trackTitle; }
    const std::string& getArtistName()   const override { return m_artistName; }
    const std::string& getCoverArtPath() const { return m_coverArtPath; }

    space::SPObservable& getPlaybackEvents() override { return m_playbackEvents; }

    wxDECLARE_EVENT_TABLE();
};

#endif
