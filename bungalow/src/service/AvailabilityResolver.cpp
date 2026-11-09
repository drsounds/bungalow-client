#include "service/AvailabilityResolver.h"

#include "service/LocalMediaService.h"
#include "service/MusicService.h"

#include <utility>

namespace space {
namespace service {

AvailabilityResolver::AvailabilityResolver(ServiceManager& services, Executor background, Executor ui)
    : m_services(services),
      m_background(background ? std::move(background) : inlineExecutor()),
      m_ui(ui ? std::move(ui) : inlineExecutor()) {}

AvailabilityResolver::~AvailabilityResolver() = default;

TrackAvailability AvailabilityResolver::availabilityFor(const PlayRequest& request) {
    if (request.isrc.empty() && request.title.empty()) return TrackAvailability{true};

    const std::string key = request.trackKey();
    auto cached = m_cache.find(key);
    if (cached != m_cache.end()) return cached->second;

    if (m_inFlight.insert(key).second) {
        m_pending.push_back(request);
        if (!m_flushScheduled) {
            // Deferred through the UI executor, so every row asked for during
            // this paint goes into one batch.
            m_flushScheduled = true;
            std::weak_ptr<bool> alive = m_alive;
            m_ui([this, alive]() {
                if (!alive.expired()) flush();
            });
        }
    }
    // An inline executor may already have answered.
    cached = m_cache.find(key);
    return cached != m_cache.end() ? cached->second : TrackAvailability{};
}

void AvailabilityResolver::flush() {
    m_flushScheduled = false;
    if (m_pending.empty()) return;

    std::vector<PlayRequest> batch = std::move(m_pending);
    m_pending.clear();
    // The worker gets its own copies; see PlaybackCoordinator::resolveAndPlay.
    const std::vector<MusicService*> services = m_services.getServices<MusicService>();
    const std::uint64_t epoch = m_epoch;
    std::weak_ptr<bool> alive = m_alive;
    Executor ui = m_ui;
    m_background([this, alive, ui, services, epoch, batch = std::move(batch)]() {
        std::vector<std::pair<std::string, TrackAvailability>> results;
        results.reserve(batch.size());
        for (const PlayRequest& request : batch)
            results.emplace_back(request.trackKey(), probe(services, request));

        ui([this, alive, epoch, results = std::move(results)]() {
            if (alive.expired() || epoch != m_epoch) return; // destroyed, or cleared meanwhile
            for (const auto& [key, availability] : results) {
                m_cache[key] = availability;
                m_inFlight.erase(key);
            }
            // Iterate a copy: a listener may subscribe or drop its subscription.
            auto listeners = m_listeners;
            for (const auto& weak : listeners)
                if (auto listener = weak.lock()) (*listener)();
        });
    });
}

TrackAvailability AvailabilityResolver::probe(const std::vector<MusicService*>& services,
                                              const PlayRequest& request) {
    TrackAvailability availability;
    availability.resolved = true;
    for (MusicService* ms : services) {
        bool found = false;
        try {
            if (!request.isrc.empty()) found = ms->lookup(request.isrc).has_value();
            if (!found && !request.title.empty())
                found = ms->lookupByTitleArtist(request.title, request.artist).has_value();
        } catch (...) {
            found = false; // a failing service just doesn't count
        }
        if (!found) continue;
        if (ms->getId() == LocalMediaService::kId)
            availability.local = true;
        else
            availability.streamServices.push_back(ms->getName());
    }
    return availability;
}

AvailabilityResolver::Subscription AvailabilityResolver::subscribe(std::function<void()> listener) {
    auto subscription = std::make_shared<std::function<void()>>(std::move(listener));
    // Prune the subscriptions that were dropped.
    std::vector<std::weak_ptr<std::function<void()>>> live;
    for (const auto& weak : m_listeners)
        if (!weak.expired()) live.push_back(weak);
    live.push_back(subscription);
    m_listeners = std::move(live);
    return subscription;
}

void AvailabilityResolver::clear() {
    ++m_epoch;
    m_cache.clear();
    m_inFlight.clear();
    m_pending.clear();
}

} // namespace service
} // namespace space
