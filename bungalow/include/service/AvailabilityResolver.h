#pragma once

#include "service/BackgroundWorker.h"
#include "service/PlayRequest.h"
#include "service/ServiceManager.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace space {
namespace service {

class MusicService;

/// Which installed services can play a track.
struct TrackAvailability {
    bool resolved = false;                   // false while the probe is pending
    bool local    = false;                   // the local-files service can play it
    std::vector<std::string> streamServices; // names of the other services that can

    bool hasStream() const { return !streamServices.empty(); }
    bool playable() const { return local || hasStream(); }
};

/// Answers "can anything play this track?" for track lists, so a row without
/// a local file isn't greyed out when another service can stream it. Each
/// MusicService is asked with its cheap lookups (lookup / lookupByTitleArtist),
/// never by loading anything.
///
/// Called while tables paint, so it never blocks: an unknown track comes back
/// unresolved and is probed on the background executor. Requests made during
/// one paint are probed as one batch, and subscribers hear about the batch
/// once. Results are cached by PlayRequest::trackKey() until clear().
///
/// Every public method is called on the UI thread.
class AvailabilityResolver {
public:
    /// Keeps a subscription alive; dropping it unsubscribes.
    using Subscription = std::shared_ptr<std::function<void()>>;

    AvailabilityResolver(ServiceManager& services, Executor background, Executor ui);
    /// Probes still in flight are dropped.
    ~AvailabilityResolver();

    AvailabilityResolver(const AvailabilityResolver&) = delete;
    AvailabilityResolver& operator=(const AvailabilityResolver&) = delete;

    /// The cached availability, or an unresolved one while it is probed.
    TrackAvailability availabilityFor(const PlayRequest& request);

    /// @p listener runs after each batch of probes, while the returned
    /// subscription is alive.
    Subscription subscribe(std::function<void()> listener);

    /// Forget every result, e.g. after the library changed. Probes in flight
    /// are discarded when they arrive.
    void clear();

private:
    void flush();
    static TrackAvailability probe(const std::vector<MusicService*>& services, const PlayRequest& request);

    ServiceManager& m_services;
    Executor m_background;
    Executor m_ui;

    std::unordered_map<std::string, TrackAvailability> m_cache;
    std::unordered_set<std::string> m_inFlight;
    std::vector<PlayRequest> m_pending;          // asked for since the last flush
    bool m_flushScheduled = false;
    std::uint64_t m_epoch = 0;                   // bumped by clear()
    std::vector<std::weak_ptr<std::function<void()>>> m_listeners;
    std::shared_ptr<bool> m_alive = std::make_shared<bool>(true);
};

} // namespace service
} // namespace space
