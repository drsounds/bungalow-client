// The service ecosystem: capability lookup and lifecycle in ServiceManager,
// cross-service resolution in PlaybackCoordinator, remembered picks in
// ResolutionStore.

#include "Database.h"
#include "service/AuthAspect.h"
#include "service/MusicCatalogueService.h"
#include "service/MusicService.h"
#include "service/PlaybackCoordinator.h"
#include "service/ResolutionStore.h"
#include "service/ServiceManager.h"

#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace space::service;

namespace {

int failures = 0;

void check(bool ok, const char* what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << "\n";
    ++failures;
}

std::vector<std::string>* g_log = nullptr;
void log(const std::string& line) { if (g_log) g_log->push_back(line); }

/// A MusicService whose catalogue is a map of ISRC → uri and title → uri.
class FakeMusicService : public MusicService {
public:
    FakeMusicService(std::string id, std::string scheme) : m_id(std::move(id)), m_scheme(std::move(scheme)) {}

    std::string getId() const override { return m_id; }
    std::string getName() const override { return "Fake " + m_id; }
    void onCreate() override { log(m_id + ":create"); }
    void onStart() override { log(m_id + ":start"); }
    void onStop() override { log(m_id + ":stop"); }
    void onDestroy() override { log(m_id + ":destroy"); }

    bool acceptsUri(const std::string& uri) const override { return uri.rfind(m_scheme, 0) == 0; }
    void loadUri(const std::string& uri) override {
        if (failLoads) { fireError("broken"); return; }
        loaded = uri;
        setPlaybackState(PlaybackState::Stopped);
    }
    void play() override { ++plays; setPlaybackState(PlaybackState::Playing); }
    void pause() override { setPlaybackState(PlaybackState::Paused); }
    void stop() override { setPlaybackState(PlaybackState::Stopped); }
    void seek(int) override {}

    std::optional<Candidate> lookup(const std::string& isrc) override {
        if (throws) throw std::runtime_error("lookup failed");
        auto it = byIsrc.find(isrc);
        if (it == byIsrc.end()) return std::nullopt;
        return Candidate{it->second, "Song", "Artist", true};
    }
    std::optional<Candidate> lookupByTitleArtist(const std::string& title, const std::string&) override {
        auto it = byTitle.find(title);
        if (it == byTitle.end()) return std::nullopt;
        return Candidate{it->second, title, "Artist", false};
    }

    std::map<std::string, std::string> byIsrc, byTitle;
    std::string loaded;
    int plays = 0;
    bool throws = false;
    bool failLoads = false;

private:
    std::string m_id, m_scheme;
};

/// Streams and needs a sign-in: the AuthAspect cross-cast case.
class FakeAuthService : public FakeMusicService, public AuthAspect {
public:
    using FakeMusicService::FakeMusicService;
    bool isAuthenticated() const override { return signedIn; }
    void login(std::function<void()> ok, std::function<void(const std::string&)>) override {
        signedIn = true;
        ok();
    }
    void logout() override { signedIn = false; }
    std::string getAccountName() const override { return signedIn ? "me" : ""; }
    bool signedIn = false;
};

/// Streams *and* browses a catalogue: both capabilities share one Service base.
class FakeCatalogueStreamer : public FakeMusicService, public MusicCatalogueService {
public:
    using FakeMusicService::FakeMusicService;
    std::string getId() const override { return FakeMusicService::getId(); }
    std::string getName() const override { return FakeMusicService::getName(); }
    std::vector<Artist> searchArtists(const std::string& q) override { return {Artist{"a1", "", q}}; }
    std::vector<Release> searchReleases(const std::string&) override { return {}; }
    std::vector<Recording> searchRecordings(const std::string&) override { return {}; }
    std::optional<Artist> getArtistByIsni(const std::string&) override { return std::nullopt; }
    std::optional<Release> getReleaseByUpc(const std::string&) override { return std::nullopt; }
    std::optional<Recording> getRecordingByIsrc(const std::string&) override { return std::nullopt; }
};

PlayRequest request(const std::string& isrc, const std::string& title = "Song",
                    const std::string& fallback = {}) {
    PlayRequest r;
    r.isrc = isrc;
    r.title = title;
    r.artist = "Artist";
    r.fallbackUri = fallback;
    return r;
}

void testServiceManager() {
    std::vector<std::string> events;
    g_log = &events;
    {
        ServiceManager manager;
        auto* local = manager.registerService(std::make_unique<FakeMusicService>("local", "file://"));
        auto* remote = manager.registerService(std::make_unique<FakeAuthService>("remote", "remote:"));
        auto* both = manager.registerAspect(std::make_unique<FakeCatalogueStreamer>("both", "both:"));

        check(manager.getServices<MusicService>().size() == 3, "every streamer is a MusicService");
        check(manager.getServices<MediaService>().size() == 3, "a MusicService is a MediaService");
        check(manager.getService<AuthAspect>() == static_cast<AuthAspect*>(remote),
              "AuthAspect is found though it isn't a Service");
        auto catalogues = manager.getServices<MusicCatalogueService>();
        check(catalogues.size() == 1 && dynamic_cast<Service*>(catalogues[0]) == both,
              "a service with two capabilities has one Service base");
        check(manager.getService<FakeMusicService>() == local, "lookup by concrete class");

        manager.startAll();
        manager.unregisterAspect("local");
        check(manager.get("local") == nullptr, "unregister removes the service");

        // Replacing an id stops and destroys the old instance.
        manager.registerAspect(std::make_unique<FakeMusicService>("remote", "remote:"));
        check(manager.all().size() == 2, "replacing an id keeps one instance");
    }
    const std::vector<std::string> expected = {
        "local:create", "remote:create", "both:create",
        "local:start", "remote:start", "both:start",
        "local:stop", "local:destroy",
        "remote:stop", "remote:destroy", "remote:create",
        // Shutdown: newest first.
        "remote:stop", "remote:destroy", "both:stop", "both:destroy",
    };
    check(events == expected, "lifecycle order");
    if (events != expected)
        for (const auto& e : events) std::cerr << "  " << e << "\n";
    g_log = nullptr;
}

void testCoordinator() {
    ServiceManager manager;
    auto* local = manager.registerService(std::make_unique<FakeMusicService>("local", "file://"));
    auto* remote = manager.registerService(std::make_unique<FakeMusicService>("remote", "remote:"));
    local->byIsrc["ONLYLOCAL"] = "file:///a.mp3";
    local->byIsrc["BOTH"] = "file:///b.mp3";
    remote->byIsrc["BOTH"] = "remote:b";
    remote->byTitle["Untagged"] = "remote:u";

    PlaybackCoordinator coordinator(manager);
    std::vector<MediaService*> activeChanges;
    coordinator.addActiveServiceListener([&](MediaService* s) { activeChanges.push_back(s); });

    int chooserCalls = 0;
    std::optional<PlaybackCoordinator::Choice> nextChoice;
    coordinator.setChooser([&](const PlayRequest&, const std::vector<ServiceMatch>& matches) {
        ++chooserCalls;
        check(matches.size() == 2, "the chooser sees one match per service");
        return nextChoice;
    });

    // One match: plays at once, no chooser.
    auto r = coordinator.resolveAndPlay(request("ONLYLOCAL"));
    check(r.played() && r.service == local && local->loaded == "file:///a.mp3", "single match plays");
    check(chooserCalls == 0, "a single match doesn't ask");
    check(coordinator.getActiveService() == local, "the playing service becomes active");

    // Title fallback when the ISRC is unknown.
    r = coordinator.resolveAndPlay(request("", "Untagged"));
    check(r.played() && remote->loaded == "remote:u", "resolves by title and artist");
    check(activeChanges.size() == 2 && activeChanges[1] == remote, "active-service listeners fire on change");

    // Several matches: the chooser decides; cancelling plays nothing.
    nextChoice = std::nullopt;
    const int playsBefore = local->plays + remote->plays;
    r = coordinator.resolveAndPlay(request("BOTH"));
    check(r.status == PlaybackCoordinator::Status::Cancelled, "cancelled chooser");
    check(local->plays + remote->plays == playsBefore, "cancelling plays nothing");

    nextChoice = PlaybackCoordinator::Choice{1, false};
    r = coordinator.resolveAndPlay(request("BOTH"));
    check(r.played() && r.service == remote && remote->loaded == "remote:b", "the chosen match plays");

    // Forcing the chooser asks even for a single match.
    chooserCalls = 0;
    nextChoice = std::nullopt;
    remote->byIsrc.erase("BOTH");
    coordinator.setChooser([&](const PlayRequest&, const std::vector<ServiceMatch>&) {
        ++chooserCalls;
        return nextChoice;
    });
    coordinator.resolveAndPlay(request("BOTH"), true);
    check(chooserCalls == 1, "Play with… always shows the chooser");
    remote->byIsrc["BOTH"] = "remote:b";

    // No matches: the fallback URI plays on a service that accepts it.
    r = coordinator.resolveAndPlay(request("NOPE", "Nope", "remote:direct"));
    check(r.played() && remote->loaded == "remote:direct", "fallback URI");
    r = coordinator.resolveAndPlay(request("NOPE", "Nope"));
    check(r.status == PlaybackCoordinator::Status::NotFound, "nothing can play it");

    // A row's own file beats the service's lookup of the same recording.
    r = coordinator.resolveAndPlay(request("ONLYLOCAL", "Song", "file:///compilation/a.mp3"));
    check(r.played() && local->loaded == "file:///compilation/a.mp3", "the request's own item is the match");

    // A throwing service is skipped; a failing load is reported.
    remote->throws = true;
    r = coordinator.resolveAndPlay(request("BOTH"));
    check(r.played() && r.service == local, "a throwing service is skipped");
    remote->throws = false;
    local->failLoads = true;
    r = coordinator.resolveAndPlay(request("ONLYLOCAL"));
    check(r.status == PlaybackCoordinator::Status::Failed, "a load error is reported");
    local->failLoads = false;

    // Direct URIs route by scheme; musik:isrc: is resolved like a request.
    check(coordinator.canPlayUri("file:///x.mp3") && !coordinator.canPlayUri("http://x"), "canPlayUri");
    r = coordinator.playUri("musik:isrc:ONLYLOCAL");
    check(r.played() && local->loaded == "file:///a.mp3", "musik:isrc: resolves across services");

    manager.unregisterAspect("local");
    check(coordinator.getActiveService() == nullptr, "an unregistered service is no longer active");
}

void testResolutionStore() {
    const auto path = std::filesystem::temp_directory_path() / "bungalow-service-test.db";
    std::filesystem::remove(path);
    {
        Database db(path.string());
        db.upsertRecording("BOTH", "Song", "");
        const int64_t trackId = db.upsertTrack("BOTH", "");

        ServiceManager manager;
        auto* local = manager.registerService(std::make_unique<FakeMusicService>("local", "file://"));
        auto* remote = manager.registerService(std::make_unique<FakeMusicService>("remote", "remote:"));
        local->byIsrc["BOTH"] = "file:///b.mp3";
        remote->byIsrc["BOTH"] = "remote:b";

        ResolutionStore store(db.handle());
        PlaybackCoordinator coordinator(manager, &store);
        int chooserCalls = 0;
        coordinator.setChooser([&](const PlayRequest&, const std::vector<ServiceMatch>&) {
            ++chooserCalls;
            return std::optional<PlaybackCoordinator::Choice>(PlaybackCoordinator::Choice{1, true});
        });

        PlayRequest fromLibrary = request("BOTH");
        fromLibrary.trackId = trackId;
        coordinator.resolveAndPlay(fromLibrary);
        check(chooserCalls == 1, "first play asks");
        auto pick = store.find(fromLibrary);
        check(pick && pick->serviceId == "remote" && pick->matchUri == "remote:b", "always remembers the pick");

        remote->loaded.clear();
        coordinator.resolveAndPlay(fromLibrary);
        check(chooserCalls == 1 && remote->loaded == "remote:b", "a remembered pick plays without asking");
        coordinator.resolveAndPlay(request("BOTH"));
        check(chooserCalls == 1, "the pick is found by key from outside the library too");

        // A pick whose service is gone falls back to resolving afresh.
        manager.unregisterAspect("remote");
        auto r = coordinator.resolveAndPlay(fromLibrary);
        check(r.played() && r.service == local, "a pick for a missing service is ignored");

        coordinator.forgetPick(fromLibrary);
        check(!store.find(fromLibrary), "forgetPick clears the pick");

        // Deleting the library track deletes the picks bound to it.
        store.save(fromLibrary, ServiceMatch{local, MusicService::Candidate{"file:///b.mp3", "Song", "", true}});
        db.deleteTrack(trackId);
        PlayRequest byTrackOnly;
        byTrackOnly.trackId = trackId;
        byTrackOnly.title = "unrelated";
        check(!store.find(byTrackOnly), "picks follow their track");
    }
    std::filesystem::remove(path);
}

} // namespace

int main() {
    testServiceManager();
    testCoordinator();
    testResolutionStore();
    if (failures == 0) std::cout << "service test passed\n";
    return failures == 0 ? 0 : 1;
}
