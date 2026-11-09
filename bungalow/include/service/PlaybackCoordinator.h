#pragma once

#include "service/BackgroundWorker.h"
#include "service/PlayRequest.h"
#include "service/ResolutionStore.h"
#include "service/ServiceMatch.h"
#include "service/ServiceManager.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace space {
namespace service {

class MediaService;
class MusicService;

/// Routes play requests to the installed Services. A track isn't bound to a
/// vendor: resolveAndPlay() asks every MusicService whether it can play the
/// request, lets the user choose ("Play with…"), and can remember that choice
/// in a ResolutionStore.
///
/// Lookups can be slow (a network service), so they run on the background
/// executor; the chooser and playback happen back on the UI executor. Every
/// public method is called on the UI thread. MusicService lookups therefore
/// run on another thread and must be safe to call alongside transport calls;
/// and a Service must not be unregistered while a resolution is in flight.
///
/// The Service that handled the most recent play is the *active* Service;
/// the transport (pause, seek, position) talks to it.
class PlaybackCoordinator {
public:
    /// The user's pick from the chooser: an index into the matches, and
    /// whether to always play this track with it.
    struct Choice {
        std::size_t index  = 0;
        bool        always = false;
    };

    /// Shows the matches and returns the user's pick, or nullopt if cancelled.
    using Chooser = std::function<std::optional<Choice>(const PlayRequest&,
                                                        const std::vector<ServiceMatch>&)>;

    enum class Status {
        Played,     // a Service is playing the request
        Cancelled,  // the user dismissed the chooser
        NotFound,   // no Service can play the request
        Failed,     // the chosen Service reported an error while loading
        Superseded, // a newer play started before this one was resolved
    };

    struct Result {
        Status        status  = Status::NotFound;
        MediaService* service = nullptr; // the Service that took the request
        std::string   uri;               // what it loaded, when known
        bool played() const { return status == Status::Played; }
    };

    /// Receives the outcome of an asynchronous play, on the UI thread.
    using Callback = std::function<void(const Result&)>;

    /// @p store may be null: picks are then never remembered.
    explicit PlaybackCoordinator(ServiceManager& services, ResolutionStore* store = nullptr);
    /// Results still in flight are dropped without calling their callbacks.
    ~PlaybackCoordinator();

    PlaybackCoordinator(const PlaybackCoordinator&) = delete;
    PlaybackCoordinator& operator=(const PlaybackCoordinator&) = delete;

    /// Without a chooser, the first match plays.
    void setChooser(Chooser chooser) { m_chooser = std::move(chooser); }

    /// Where lookups run, and how their results get back to the UI thread.
    /// Both run inline until set, which makes every play synchronous.
    void setExecutors(Executor background, Executor ui);

    /// Play @p request. A remembered pick plays straight away; otherwise the
    /// matches of every MusicService are gathered in the background and shown
    /// in the chooser — even a single match, so the user can pin it — and no
    /// match falls back to the request's fallbackUri. @p forceChooser ignores
    /// the remembered pick: the "Play with…" action.
    /// @p done gets the outcome, possibly before this returns.
    void resolveAndPlay(const PlayRequest& request, bool forceChooser, Callback done);

    /// Make any resolution still in flight finish as Superseded.
    void cancelPending() { ++m_generation; }

    /// One match per candidate of every MusicService, in registration order.
    /// A Service that accepts the request's fallbackUri offers exactly that
    /// item. A Service that throws is skipped. Blocks on the lookups.
    std::vector<ServiceMatch> gatherMatches(const PlayRequest& request) const;

    /// Play @p uri. A musik:isrc:<ISRC> URI names a recording, not a source,
    /// so it is resolved like a request; any other URI plays at once on the
    /// first MediaService that accepts it.
    void playUri(const std::string& uri, Callback done);

    /// Play @p request on @p service, bypassing resolution.
    Result playOn(MusicService& service, const PlayRequest& request);

    /// Whether playUri() could play @p uri.
    bool canPlayUri(const std::string& uri) const;

    /// The first MediaService that accepts @p uri, or nullptr.
    MediaService* serviceForUri(const std::string& uri) const;

    /// Forget the remembered pick for @p request.
    void forgetPick(const PlayRequest& request);

    /// The Service that handled the most recent play, or nullptr. Cleared
    /// when that Service is no longer registered.
    MediaService* getActiveService() const;

    /// Called with the new active Service whenever it changes.
    void addActiveServiceListener(std::function<void(MediaService*)> listener);

    /// The PlayRequest a musik:isrc:<ISRC> URI names, or nullopt.
    static std::optional<PlayRequest> requestForContentUri(const std::string& uri);

private:
    static std::vector<ServiceMatch> gather(const std::vector<MusicService*>& services,
                                            const PlayRequest& request);
    Result choose(std::uint64_t generation, const PlayRequest& request, std::vector<ServiceMatch> matches);
    Result playMatch(const ServiceMatch& match, const PlayRequest& request);
    Result playPick(const ResolutionStore::Pick& pick);
    Result playFallback(const PlayRequest& request);
    Result started(MediaService& service, const std::string& uri);
    MusicService* findMusicService(const std::string& id) const;
    void setActiveService(MediaService* service);

    ServiceManager& m_services;
    ResolutionStore* m_store;
    Chooser m_chooser;
    Executor m_background = inlineExecutor();
    Executor m_ui = inlineExecutor();
    std::uint64_t m_generation = 0; // bumped by every play; older resolutions are superseded
    MediaService* m_active = nullptr;
    std::vector<std::function<void(MediaService*)>> m_activeListeners;
    std::shared_ptr<bool> m_alive = std::make_shared<bool>(true); // results check it on arrival
};

} // namespace service
} // namespace space
