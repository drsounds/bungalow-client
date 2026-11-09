#pragma once

#include "service/PlayRequest.h"
#include "service/ResolutionStore.h"
#include "service/ServiceMatch.h"
#include "service/ServiceManager.h"

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace space {
namespace service {

class MediaService;
class MusicService;

/// Routes play requests to the installed Services. A track isn't bound to a
/// vendor: resolveAndPlay() asks every MusicService whether it can play the
/// request, lets the user choose when several can ("Play with…"), and can
/// remember that choice in a ResolutionStore.
///
/// The Service that handled the most recent play is the *active* Service;
/// the transport (pause, seek, position) talks to it.
///
/// Lookups run on the calling thread, so MusicServices should keep lookup()
/// and lookupByTitleArtist() quick.
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
    };

    struct Result {
        Status        status  = Status::NotFound;
        MediaService* service = nullptr; // the Service that took the request
        std::string   uri;               // what it loaded, when known
        bool played() const { return status == Status::Played; }
    };

    /// @p store may be null: picks are then never remembered.
    explicit PlaybackCoordinator(ServiceManager& services, ResolutionStore* store = nullptr);

    /// Without a chooser, the first match plays.
    void setChooser(Chooser chooser) { m_chooser = std::move(chooser); }

    /// Play @p request: a remembered pick plays straight away; otherwise the
    /// matches of every MusicService are gathered. One match plays at once,
    /// several go to the chooser, none falls back to the request's
    /// fallbackUri. @p forceChooser ignores the remembered pick and shows the
    /// chooser even for a single match — the "Play with…" action.
    Result resolveAndPlay(const PlayRequest& request, bool forceChooser = false);

    /// One match per candidate of every MusicService, in registration order.
    /// A Service that accepts the request's fallbackUri offers exactly that
    /// item. A Service that throws is skipped.
    std::vector<ServiceMatch> gatherMatches(const PlayRequest& request) const;

    /// Play @p uri. A musik:isrc:<ISRC> URI names a recording, not a source,
    /// so it is resolved like a request; any other URI plays on the first
    /// MediaService that accepts it.
    Result playUri(const std::string& uri);

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
    Result playMatch(const ServiceMatch& match, const PlayRequest& request);
    Result playPick(const ResolutionStore::Pick& pick);
    Result playFallback(const PlayRequest& request);
    Result started(MediaService& service, const std::string& uri);
    MusicService* findMusicService(const std::string& id) const;
    void setActiveService(MediaService* service);

    ServiceManager& m_services;
    ResolutionStore* m_store;
    Chooser m_chooser;
    MediaService* m_active = nullptr;
    std::vector<std::function<void(MediaService*)>> m_activeListeners;
};

} // namespace service
} // namespace space
