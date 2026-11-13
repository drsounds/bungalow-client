#ifndef SPACE_APP_H
#define SPACE_APP_H

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

#include <memory>
#include <string>
#include <vector>

namespace space
{
    class Space;
    class SPWindowElement;
    namespace ui { class MainWindow; }
}

// Win32 counterpart of the wxWidgets BungalowApp: owns the database, download
// manager, services and playback queue, and builds the MainWindow UI. Each
// queue entry is resolved to a service at play time by the PlaybackCoordinator.
class BungalowApp : public space::SPApplicationContext
{
protected:
    space::Space* m_space = nullptr;
    space::SPWindowElement* m_windowElement = nullptr;
    space::ui::MainWindow* m_main = nullptr;
    std::unique_ptr<Database> m_database;
    std::string m_databasePath;
    std::unique_ptr<DownloadManager> m_downloadManager;

    // Declared after m_database so they are destroyed before it.
    space::service::ServiceManager                       m_services;
    std::unique_ptr<space::service::ResolutionStore>     m_resolutionStore;
    std::unique_ptr<space::service::PlaybackCoordinator> m_coordinator;
    // Views hold weak references, so they never outlive it unknowingly.
    std::shared_ptr<space::service::AvailabilityResolver> m_availability;
    // Where service lookups run, one at a time.
    std::unique_ptr<space::service::BackgroundWorker>    m_worker;
    // Externally loaded plugins (<app>/Apps, ~/Bungalow) and the services they
    // registered. Declared after m_services, so it is destroyed before it.
    std::unique_ptr<space::plugin::PluginManager>        m_plugins;

    bool m_developerMode = false;

    bool                        m_hasCurrent = false; // a queue entry is loaded on a service
    std::vector<PlayQueueEntry> m_queueEntries;
    int                         m_queueIndex = -1;

    bool                     m_isPlaying  = false;
    int                      m_positionMs = 0;
    int                      m_durationMs = 0;

    std::string m_trackTitle;
    std::string m_artistName;
    std::string m_coverArtPath;

    space::SPObservable m_playbackEvents;

    unsigned long long m_timerId = 0;
    void startPositionTimer();
    void stopPositionTimer();

    void dispatchPlaybackChanged();
    void stopCurrentPlayback();
    void playFromIndex(int idx, bool chooseService = false);
    void onResolved(int idx, const space::service::PlaybackCoordinator::Result& result);
    bool canResolve(const PlayQueueEntry& entry) const;

public:
    static BungalowApp* current() { return s_instance; }

    BungalowApp();
    ~BungalowApp() override;

    // Builds the UI inside windowElement and loads settings. Returns the MainWindow.
    space::ui::MainWindow* initialize(space::Space* space, space::SPWindowElement* windowElement);
    void shutdown();
    void onPositionTimer();

    Database* getDatabase() { return m_database.get(); }
    const std::string& getDatabasePath() const { return m_databasePath; }
    DownloadManager* getDownloadManager() { return m_downloadManager.get(); }

    // Installed services; select them by capability with getServices<T>().
    space::service::ServiceManager& getServiceManager() { return m_services; }
    // Null before the services are set up.
    space::plugin::PluginManager* getPluginManager() { return m_plugins.get(); }
    // Null before initialize().
    space::service::PlaybackCoordinator* getPlaybackCoordinator() { return m_coordinator.get(); }
    // The service that is playing (or last played), or null.
    space::service::MediaService* getActiveService() const;
    // Which services can play a track, for greying out track lists. Empty
    // before initialize() and after shutdown().
    std::weak_ptr<space::service::AvailabilityResolver> getAvailabilityResolver() const { return m_availability; }

    void navigate(const std::string& uri);

    bool isDeveloperMode() const { return m_developerMode; }
    void setDeveloperMode(bool enabled);
    void setFullWidthAddressBar(bool fullWidth);
    void showWebInspector();
    void reloadWebPage();

    void showError(const std::string& msg = "An error occurred");

    void playContext(PlayContext ctx);
    void playQueueAtIndex(int idx);
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

private:
    static BungalowApp* s_instance;
};

#endif
