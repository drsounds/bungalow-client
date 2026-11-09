#include "service/PlaybackCoordinator.h"

#include "service/MediaService.h"
#include "service/MusicService.h"

#include <algorithm>
#include <exception>

namespace space {
namespace service {

namespace {
const std::string kMusikIsrcPrefix = "musik:isrc:";
}

PlaybackCoordinator::PlaybackCoordinator(ServiceManager& services, ResolutionStore* store)
    : m_services(services), m_store(store) {}

PlaybackCoordinator::~PlaybackCoordinator() = default;

void PlaybackCoordinator::setExecutors(Executor background, Executor ui) {
    m_background = background ? std::move(background) : inlineExecutor();
    m_ui = ui ? std::move(ui) : inlineExecutor();
}

// ── Resolution ────────────────────────────────────────────────────────────────

void PlaybackCoordinator::resolveAndPlay(const PlayRequest& request, bool forceChooser, Callback done) {
    const std::uint64_t generation = ++m_generation;
    if (!done) done = [](const Result&) {};

    if (!forceChooser && m_store) {
        if (auto pick = m_store->find(request)) {
            Result r = playPick(*pick);
            // A pick whose Service is gone or can't play it any more falls
            // through to a fresh resolution.
            if (r.played()) {
                done(r);
                return;
            }
        }
    }

    // The worker gets its own copies: it must not touch the coordinator, which
    // may be gone by the time the lookups finish.
    const std::vector<MusicService*> services = m_services.getServices<MusicService>();
    std::weak_ptr<bool> alive = m_alive;
    Executor ui = m_ui;
    m_background([this, alive, ui, services, request, generation, done]() {
        std::vector<ServiceMatch> matches = gather(services, request);
        ui([this, alive, request, generation, done, matches = std::move(matches)]() mutable {
            if (alive.expired()) return;
            done(choose(generation, request, std::move(matches)));
        });
    });
}

PlaybackCoordinator::Result PlaybackCoordinator::choose(std::uint64_t generation, const PlayRequest& request,
                                                        std::vector<ServiceMatch> matches) {
    if (generation != m_generation) return Result{Status::Superseded};

    // A Service may have been unregistered while the lookups ran.
    const auto registered = m_services.getServices<MusicService>();
    matches.erase(std::remove_if(matches.begin(), matches.end(),
                                 [&](const ServiceMatch& m) {
                                     return std::find(registered.begin(), registered.end(), m.service) ==
                                            registered.end();
                                 }),
                  matches.end());

    if (matches.empty()) return playFallback(request);
    if (!m_chooser) return playMatch(matches.front(), request);

    // Unresolved tracks always go to the chooser, even with one match, so the
    // user learns which Service plays them and can pin that choice.
    const std::optional<Choice> choice = m_chooser(request, matches);
    // The chooser is modal and pumps messages: a newer play may have started.
    if (generation != m_generation) return Result{Status::Superseded};
    if (!choice || choice->index >= matches.size()) return Result{Status::Cancelled};

    const ServiceMatch& match = matches[choice->index];
    Result r = playMatch(match, request);
    if (r.played() && choice->always && m_store) m_store->save(request, match);
    return r;
}

std::vector<ServiceMatch> PlaybackCoordinator::gatherMatches(const PlayRequest& request) const {
    return gather(m_services.getServices<MusicService>(), request);
}

std::vector<ServiceMatch> PlaybackCoordinator::gather(const std::vector<MusicService*>& services,
                                                      const PlayRequest& request) {
    std::vector<ServiceMatch> matches;
    for (MusicService* ms : services) {
        try {
            // The request already names an item this Service plays (the row's
            // own file, say): that item is its match, not whatever a lookup
            // of the same recording would find.
            if (!request.fallbackUri.empty() && ms->acceptsUri(request.fallbackUri)) {
                matches.push_back(ServiceMatch{
                    ms, MusicService::Candidate{request.fallbackUri, request.title, request.artist, false}});
                continue;
            }
            for (auto& candidate : ms->findCandidates(request.isrc, request.title, request.artist))
                matches.push_back(ServiceMatch{ms, std::move(candidate)});
        } catch (const std::exception&) {
            // One failing Service must not hide the others' matches.
        }
    }
    return matches;
}

PlaybackCoordinator::Result PlaybackCoordinator::playMatch(const ServiceMatch& match,
                                                           const PlayRequest& request) {
    MusicService& ms = *match.service;
    if (!match.candidate.uri.empty())
        ms.loadUri(match.candidate.uri);  // the exact item the Service offered
    else if (match.candidate.byIsrc && !request.isrc.empty())
        ms.loadByIsrc(request.isrc);
    else
        ms.loadByTitleArtist(request.title, request.artist);
    return started(ms, match.candidate.uri);
}

PlaybackCoordinator::Result PlaybackCoordinator::playPick(const ResolutionStore::Pick& pick) {
    MusicService* ms = findMusicService(pick.serviceId);
    if (!ms) return Result{Status::NotFound};
    // Prefer the exact saved item so the pick replays precisely.
    if (!pick.matchUri.empty())
        ms->loadUri(pick.matchUri);
    else if (!pick.matchIsrc.empty())
        ms->loadByIsrc(pick.matchIsrc);
    else if (!pick.matchTitle.empty())
        ms->loadByTitleArtist(pick.matchTitle, pick.matchArtist);
    else
        return Result{Status::NotFound};
    return started(*ms, pick.matchUri);
}

PlaybackCoordinator::Result PlaybackCoordinator::playFallback(const PlayRequest& request) {
    if (request.fallbackUri.empty()) return Result{Status::NotFound};
    MediaService* ms = serviceForUri(request.fallbackUri);
    if (!ms) return Result{Status::NotFound};
    ms->loadUri(request.fallbackUri);
    return started(*ms, request.fallbackUri);
}

PlaybackCoordinator::Result PlaybackCoordinator::started(MediaService& service, const std::string& uri) {
    // load*() reports a failure through the Error state rather than a return value.
    if (service.getPlaybackState() == MediaService::PlaybackState::Error)
        return Result{Status::Failed, &service, uri};
    service.play();
    setActiveService(&service);
    return Result{Status::Played, &service, uri};
}

// ── Direct playback ───────────────────────────────────────────────────────────

std::optional<PlayRequest> PlaybackCoordinator::requestForContentUri(const std::string& uri) {
    if (uri.compare(0, kMusikIsrcPrefix.size(), kMusikIsrcPrefix) != 0) return std::nullopt;
    PlayRequest request;
    request.isrc = uri.substr(kMusikIsrcPrefix.size());
    if (request.isrc.empty()) return std::nullopt;
    return request;
}

void PlaybackCoordinator::playUri(const std::string& uri, Callback done) {
    if (!done) done = [](const Result&) {};
    if (auto request = requestForContentUri(uri)) {
        resolveAndPlay(*request, false, std::move(done));
        return;
    }
    ++m_generation;
    MediaService* ms = serviceForUri(uri);
    if (!ms) {
        done(Result{Status::NotFound});
        return;
    }
    ms->loadUri(uri);
    done(started(*ms, uri));
}

PlaybackCoordinator::Result PlaybackCoordinator::playOn(MusicService& service, const PlayRequest& request) {
    ++m_generation;
    if (!request.isrc.empty())
        service.loadByIsrc(request.isrc);
    else
        service.loadByTitleArtist(request.title, request.artist);
    return started(service, {});
}

bool PlaybackCoordinator::canPlayUri(const std::string& uri) const {
    if (requestForContentUri(uri)) return !m_services.getServices<MusicService>().empty();
    return serviceForUri(uri) != nullptr;
}

MediaService* PlaybackCoordinator::serviceForUri(const std::string& uri) const {
    for (MediaService* ms : m_services.getServices<MediaService>())
        if (ms->acceptsUri(uri)) return ms;
    return nullptr;
}

void PlaybackCoordinator::forgetPick(const PlayRequest& request) {
    if (m_store) m_store->clear(request);
}

// ── Active Service ────────────────────────────────────────────────────────────

MediaService* PlaybackCoordinator::getActiveService() const {
    // The Service may have been unregistered since it last played.
    const auto services = m_services.getServices<MediaService>();
    return std::find(services.begin(), services.end(), m_active) != services.end() ? m_active : nullptr;
}

void PlaybackCoordinator::addActiveServiceListener(std::function<void(MediaService*)> listener) {
    if (listener) m_activeListeners.push_back(std::move(listener));
}

void PlaybackCoordinator::setActiveService(MediaService* service) {
    if (service == m_active) return;
    m_active = service;
    for (const auto& listener : std::vector<std::function<void(MediaService*)>>(m_activeListeners))
        listener(service);
}

MusicService* PlaybackCoordinator::findMusicService(const std::string& id) const {
    for (MusicService* ms : m_services.getServices<MusicService>())
        if (ms->getId() == id) return ms;
    return nullptr;
}

} // namespace service
} // namespace space
