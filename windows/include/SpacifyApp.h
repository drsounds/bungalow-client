#ifndef SPACE_APP_H
#define SPACE_APP_H

#include "PlayContext.h"
#include "PlayQueueEntry.h"
#include "Database.h"
#include "SPObservable.h"
#include "SPApplicationContext.h"
#include "DownloadManager.h"

#include <memory>
#include <string>
#include <vector>

class SPMediaService;
class SPPlayable;

namespace space
{
    class Space;
    class SPWindowElement;
    class MainElement;
}

// Win32 counterpart of the wxWidgets BungalowApp: owns the database, download
// manager, media services and playback queue, and builds the MainElement UI.
class BungalowApp : public space::SPApplicationContext
{
protected:
    space::Space* m_space = nullptr;
    space::SPWindowElement* m_windowElement = nullptr;
    space::MainElement* m_main = nullptr;
    std::unique_ptr<Database> m_database;
    std::string m_databasePath;
    std::unique_ptr<DownloadManager> m_downloadManager;

    std::vector<SPMediaService*> m_mediaServices;

    bool m_developerMode = false;

    SPPlayable*              m_currentPlayable = nullptr;
    std::vector<PlayQueueEntry> m_queueEntries;
    std::vector<SPPlayable*>    m_queuePlayables;
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
    void playFromIndex(int idx);

public:
    static BungalowApp* current() { return s_instance; }

    BungalowApp();
    ~BungalowApp() override;

    // Builds the UI inside windowElement and loads settings. Returns the MainElement.
    space::MainElement* initialize(space::Space* space, space::SPWindowElement* windowElement);
    void shutdown();
    void onPositionTimer();

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

    void showError(const std::string& msg = "An error occurred");

    void playContext(PlayContext ctx);
    void playQueueAtIndex(int idx);
    void playTrackId(int64_t trackId);
    void playUri(const std::string& uri);

    void togglePlayPause() override;
    void seekTo(int posMs) override;
    void playbackNext() override;
    void playbackPrev() override;

    bool        isPlaying()    const { return m_isPlaying; }
    int         getPositionMs()const { return m_positionMs; }
    int         getDurationMs()const { return m_durationMs; }
    SPPlayable* getCurrentPlayable() const { return m_currentPlayable; }
    bool hasCurrentPlayable() const override { return m_currentPlayable != nullptr; }
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
