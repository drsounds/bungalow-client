// The service ecosystem: capability lookup and lifecycle in ServiceManager,
// cross-service resolution in PlaybackCoordinator (synchronous and in the
// background), remembered picks in ResolutionStore, track availability in
// AvailabilityResolver, and the BackgroundWorker they run on.

#include "Database.h"
#include "service/AuthAspect.h"
#include "service/AvailabilityResolver.h"
#include "service/BackgroundWorker.h"
#include "service/MusicCatalogueService.h"
#include "service/MusicService.h"
#include "service/PlaybackCoordinator.h"
#include "service/ResolutionStore.h"
#include "service/ServiceManager.h"
#include "service/SessionCipher.h"

#include <nlohmann/json.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace space::service;
using Status = PlaybackCoordinator::Status;

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
        ++lookups;
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
    int lookups = 0;
    bool throws = false;
    bool failLoads = false;

private:
    std::string m_id, m_scheme;
};

/// Streams and needs a sign-in: the AuthAspect cross-cast case.
class FakeAuthService : public FakeMusicService {
public:
    using FakeMusicService::FakeMusicService;
    bool supportsLogin() const override { return true; }
    bool isLoggedIn() const override { return signedIn; }
    /// Asynchronous like a real back-end: the result arrives from pending().
    void login(std::function<void(const SPSession&)> ok,
               std::function<void(const std::string&)> err) override {
        pending = [this, ok, err] {
            if (fail) {
                err("denied");
                return;
            }
            signedIn = true;
            ok(getSession());
        };
    }
    void logout() override { signedIn = false; }
    SPSession getSession() const override {
        if (!signedIn) return {};
        SPSession session(SPUser{"u1", "me"}, getId());
        session.setData({{"token", "secret"}});
        return session;
    }
    bool signedIn = false;
    bool fail = false;
    std::function<void()> pending;
};
    }
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

/// An executor that queues tasks until the test runs them.
struct ManualExecutor {
    std::vector<std::function<void()>> tasks;
    Executor executor() {
        return [this](std::function<void()> task) { tasks.push_back(std::move(task)); };
    }
    /// Runs everything queued, including tasks queued while running.
    int drain() {
        int ran = 0;
        while (!tasks.empty()) {
            auto batch = std::move(tasks);
            tasks.clear();
            for (auto& task : batch) { task(); ++ran; }
        }
        return ran;
    }
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

/// resolveAndPlay with inline executors, returning its result.
PlaybackCoordinator::Result play(PlaybackCoordinator& coordinator, const PlayRequest& req, bool force = false) {
    PlaybackCoordinator::Result out{Status::Superseded};
    bool called = false;
    coordinator.resolveAndPlay(req, force, [&](const PlaybackCoordinator::Result& r) {
        out = r;
        called = true;
    });
    check(called, "inline executors resolve synchronously");
    return out;
}

/// Reversible, obviously-not-clear-text stand-in so the test runs anywhere.
class XorCipher : public SessionCipher {
public:
    bool encrypt(const std::string& in, std::string& out) override { out = xor_(in); return true; }
    bool decrypt(const std::string& in, std::string& out) override { out = xor_(in); return true; }
private:
    static std::string xor_(std::string s) { for (char& c : s) c ^= 0x5a; return s; }
};

void testSession() {
    SessionCipher::setInstance(std::make_shared<XorCipher>());
    SPSession session(SPUser{"u1", "me"}, "svc");
    check(session.isValid() && session.serviceId() == "svc", "session is tied to its user and service");
    check(!session.hasData(), "no data yet");
    check(session.setData({{"token", "secret"}}), "data is stored");
    check(session.encryptedData().find("secret") == std::string::npos, "stored data is not clear text");
    bool ok = false;
    nlohmann::json data = session.data(&ok);
    check(ok && data["token"] == "secret", "data decrypts back to the JSON");

    SPSession restored(SPUser{"u1", "me"}, "svc");
    restored.setEncryptedData(session.encryptedData());
    check(restored.data()["token"] == "secret", "an encrypted blob restores a session");

    SessionCipher::setInstance(nullptr);
    check(!session.setData({{"a", 1}}), "without a cipher no data is kept");
    session.data(&ok);
    check(!ok, "without a cipher data cannot be read");
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
        check(manager.getServices<AuthAspect>().size() == 3, "every Service is an AuthAspect");
        check(!local->supportsLogin() && !local->isLoggedIn() && !local->getSession().isValid(),
              "a service without auth stays signed out");

        // Sign-in is asynchronous and lands in the AccountManager.
        std::string loginResult;
        manager.login("remote", [&](const SPSession& s) { loginResult = "ok:" + s.user().name; },
                      [&](const std::string& e) { loginResult = "err:" + e; });
        check(loginResult.empty() && !remote->isLoggedIn() && !manager.accounts().isSignedIn("remote"),
              "login returns before the sign-in finishes");
        remote->pending();
        check(loginResult == "ok:me" && manager.accounts().isSignedIn("remote") &&
                  remote->getAccountName() == "me",
              "a successful login records the session");
        check(manager.accounts().all().size() == 1, "one account stored");

        manager.logout("remote");
        check(!remote->isLoggedIn() && !manager.accounts().isSignedIn("remote"),
              "logout forgets the account");

        remote->fail = true;
        manager.login("remote", nullptr, [&](const std::string& e) { loginResult = "err:" + e; });
        remote->pending();
        check(loginResult == "err:denied" && !manager.accounts().isSignedIn("remote"),
              "a failed login stores nothing");
        manager.login("nope", nullptr, [&](const std::string& e) { loginResult = e; });
        check(loginResult.rfind("Unknown service", 0) == 0, "unknown service reports an error");
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

    // Without a chooser, the first match plays.
    auto r = play(coordinator, request("BOTH"));
    check(r.played() && r.service == local, "no chooser: first match");

    int chooserCalls = 0;
    std::size_t lastMatchCount = 0;
    std::optional<PlaybackCoordinator::Choice> nextChoice = PlaybackCoordinator::Choice{0, false};
    coordinator.setChooser([&](const PlayRequest&, const std::vector<ServiceMatch>& matches) {
        ++chooserCalls;
        lastMatchCount = matches.size();
        return nextChoice;
    });

    // A single match still goes to the chooser, so it can be pinned.
    r = play(coordinator, request("ONLYLOCAL"));
    check(chooserCalls == 1 && lastMatchCount == 1, "a single match asks too");
    check(r.played() && r.service == local && local->loaded == "file:///a.mp3", "single match plays");
    check(coordinator.getActiveService() == local, "the playing service becomes active");

    // Title fallback when the ISRC is unknown.
    r = play(coordinator, request("", "Untagged"));
    check(r.played() && remote->loaded == "remote:u", "resolves by title and artist");
    check(activeChanges.size() == 2 && activeChanges[1] == remote, "active-service listeners fire on change");

    // Several matches: the chooser decides; cancelling plays nothing.
    nextChoice = std::nullopt;
    const int playsBefore = local->plays + remote->plays;
    r = play(coordinator, request("BOTH"));
    check(r.status == Status::Cancelled && lastMatchCount == 2, "cancelled chooser");
    check(local->plays + remote->plays == playsBefore, "cancelling plays nothing");

    nextChoice = PlaybackCoordinator::Choice{1, false};
    r = play(coordinator, request("BOTH"));
    check(r.played() && r.service == remote && remote->loaded == "remote:b", "the chosen match plays");

    // No matches: the fallback URI plays on a service that accepts it.
    nextChoice = PlaybackCoordinator::Choice{0, false};
    r = play(coordinator, request("NOPE", "Nope", "remote:direct"));
    check(r.played() && remote->loaded == "remote:direct", "fallback URI");
    r = play(coordinator, request("NOPE", "Nope"));
    check(r.status == Status::NotFound, "nothing can play it");

    // A row's own file beats the service's lookup of the same recording.
    r = play(coordinator, request("ONLYLOCAL", "Song", "file:///compilation/a.mp3"));
    check(r.played() && local->loaded == "file:///compilation/a.mp3", "the request's own item is the match");

    // A throwing service is skipped; a failing load is reported.
    remote->throws = true;
    r = play(coordinator, request("BOTH"));
    check(r.played() && r.service == local && lastMatchCount == 1, "a throwing service is skipped");
    remote->throws = false;
    local->failLoads = true;
    r = play(coordinator, request("ONLYLOCAL"));
    check(r.status == Status::Failed, "a load error is reported");
    local->failLoads = false;

    // Direct URIs route by scheme; musik:isrc: is resolved like a request.
    check(coordinator.canPlayUri("file:///x.mp3") && !coordinator.canPlayUri("http://x"), "canPlayUri");
    bool uriPlayed = false;
    coordinator.playUri("musik:isrc:ONLYLOCAL", [&](const PlaybackCoordinator::Result& res) { uriPlayed = res.played(); });
    check(uriPlayed && local->loaded == "file:///a.mp3", "musik:isrc: resolves across services");

    manager.unregisterAspect("local");
    check(coordinator.getActiveService() == nullptr, "an unregistered service is no longer active");
}

void testBackgroundResolution() {
    ServiceManager manager;
    auto* local = manager.registerService(std::make_unique<FakeMusicService>("local", "file://"));
    local->byIsrc["A"] = "file:///a.mp3";
    local->byIsrc["B"] = "file:///b.mp3";

    ManualExecutor background, ui;
    std::vector<Status> results;
    {
        PlaybackCoordinator coordinator(manager);
        coordinator.setExecutors(background.executor(), ui.executor());
        coordinator.setChooser([](const PlayRequest&, const std::vector<ServiceMatch>&) {
            return std::optional<PlaybackCoordinator::Choice>(PlaybackCoordinator::Choice{0, false});
        });
        auto record = [&](const PlaybackCoordinator::Result& r) { results.push_back(r.status); };

        coordinator.resolveAndPlay(request("A"), false, record);
        check(results.empty() && local->lookups == 0, "lookups wait for the background executor");
        background.drain();
        check(local->lookups == 1 && results.empty(), "lookups ran; the result waits for the UI");
        ui.drain();
        check(results.size() == 1 && results[0] == Status::Played && local->loaded == "file:///a.mp3",
              "the result plays on the UI executor");

        // A newer play supersedes one still being resolved.
        results.clear();
        coordinator.resolveAndPlay(request("A"), false, record);
        coordinator.resolveAndPlay(request("B"), false, record);
        background.drain();
        ui.drain();
        check(results.size() == 2 && results[0] == Status::Superseded && results[1] == Status::Played,
              "the older resolution is superseded");
        check(local->loaded == "file:///b.mp3", "only the newer request plays");

        // cancelPending supersedes too.
        results.clear();
        coordinator.resolveAndPlay(request("A"), false, record);
        coordinator.cancelPending();
        background.drain();
        ui.drain();
        check(results.size() == 1 && results[0] == Status::Superseded, "cancelPending");

        // Destroyed before its result arrives: the result is dropped.
        results.clear();
        coordinator.resolveAndPlay(request("A"), false, record);
        background.drain();
    }
    ui.drain();
    check(results.empty(), "a destroyed coordinator drops late results");
}

void testResolutionStore() {
    const auto path = std::filesystem::temp_directory_path() / "bungalow-service-test.db";
    std::filesystem::remove(path);
    {
        Database db(path.string());
        db.upsertRecording("BOTH", "Song", "");
        const int64_t trackId = db.upsertTrack("BOTH", "");

        // A read-only connection beside the writer sees its data.
        Database reader(path.string(), Database::OpenMode::ReadOnly);
        check(reader.getTracksForRecording("BOTH").size() == 1, "read-only connection reads");
        check(reader.getTracksByTitle("song").size() == 1, "title lookup ignores case");

        ServiceManager manager;
        auto* local = manager.registerService(std::make_unique<FakeMusicService>("local", "file://"));
        auto* remote = manager.registerService(std::make_unique<FakeMusicService>("remote", "remote:"));
        local->byIsrc["BOTH"] = "file:///b.mp3";
        remote->byIsrc["BOTH"] = "remote:b";

        ResolutionStore store(db.handle());
        PlaybackCoordinator coordinator(manager, &store);
        int chooserCalls = 0;
        // Always picks the last match (remote, while it is installed).
        coordinator.setChooser([&](const PlayRequest&, const std::vector<ServiceMatch>& matches) {
            ++chooserCalls;
            return std::optional<PlaybackCoordinator::Choice>(PlaybackCoordinator::Choice{matches.size() - 1, true});
        });

        PlayRequest fromLibrary = request("BOTH");
        fromLibrary.trackId = trackId;
        play(coordinator, fromLibrary);
        check(chooserCalls == 1, "first play asks");
        auto pick = store.find(fromLibrary);
        check(pick && pick->serviceId == "remote" && pick->matchUri == "remote:b", "always remembers the pick");

        remote->loaded.clear();
        play(coordinator, fromLibrary);
        check(chooserCalls == 1 && remote->loaded == "remote:b", "a remembered pick plays without asking");
        play(coordinator, request("BOTH"));
        check(chooserCalls == 1, "the pick is found by key from outside the library too");
        play(coordinator, fromLibrary, true);
        check(chooserCalls == 2, "Play with… asks despite the pick");

        // A pick whose service is gone falls back to resolving afresh.
        manager.unregisterAspect("remote");
        auto r = play(coordinator, fromLibrary);
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

void testAvailability() {
    ServiceManager manager;
    auto* local = manager.registerService(std::make_unique<FakeMusicService>("local", "file://"));
    auto* remote = manager.registerService(std::make_unique<FakeMusicService>("remote", "remote:"));
    local->byIsrc["MINE"] = "file:///m.mp3";
    remote->byIsrc["STREAM"] = "remote:s";
    remote->byTitle["Untagged"] = "remote:u";

    ManualExecutor background, ui;
    AvailabilityResolver resolver(manager, background.executor(), ui.executor());
    int notified = 0;
    auto subscription = resolver.subscribe([&] { ++notified; });

    check(!resolver.availabilityFor(request("MINE")).resolved, "unknown tracks start unresolved");
    check(!resolver.availabilityFor(request("STREAM")).resolved, "second request joins the batch");
    check(!resolver.availabilityFor(request("MINE")).resolved, "asking again doesn't probe twice");
    ui.drain();                       // the deferred flush
    check(background.tasks.size() == 1, "requests from one paint are probed as one batch");
    background.drain();
    ui.drain();
    check(notified == 1, "subscribers hear once per batch");
    check(local->lookups == 2 && remote->lookups == 2, "each track was probed once per service");

    auto mine = resolver.availabilityFor(request("MINE"));
    check(mine.resolved && mine.local && !mine.hasStream() && mine.playable(), "local availability");
    auto stream = resolver.availabilityFor(request("STREAM"));
    check(stream.resolved && !stream.local && stream.streamServices == std::vector<std::string>{"Fake remote"},
          "stream availability names the service");
    check(background.tasks.empty() && ui.tasks.empty(), "cached answers don't probe");

    // Title fallback, and nothing at all.
    resolver.availabilityFor(request("", "Untagged"));
    resolver.availabilityFor(request("NONE", "Nothing"));
    ui.drain();
    background.drain();
    ui.drain();
    check(resolver.availabilityFor(request("", "Untagged")).hasStream(), "title lookups count");
    auto none = resolver.availabilityFor(request("NONE", "Nothing"));
    check(none.resolved && !none.playable(), "an unplayable track resolves as such");

    // Results probed before clear() are discarded.
    resolver.availabilityFor(request("LATE"));
    ui.drain();
    background.drain();
    resolver.clear();
    ui.drain();
    check(!resolver.availabilityFor(request("MINE")).resolved, "clear forgets results");
    check(!resolver.availabilityFor(request("LATE")).resolved, "stale probes are dropped");

    // A dropped subscription isn't called.
    subscription.reset();
    const int before = notified;
    ui.drain();
    background.drain();
    ui.drain();
    check(notified == before, "unsubscribed listeners aren't called");
}

void testBackgroundWorker() {
    std::mutex m;
    std::condition_variable cv;
    std::vector<int> order;
    std::thread::id workerThread;
    {
        BackgroundWorker worker;
        for (int i = 0; i < 5; ++i) {
            worker.post([&, i] {
                std::lock_guard<std::mutex> lock(m);
                order.push_back(i);
                workerThread = std::this_thread::get_id();
                cv.notify_all();
            });
        }
        worker.post([] { throw std::runtime_error("ignored"); });
        worker.executor()([&] {
            std::lock_guard<std::mutex> lock(m);
            order.push_back(99);
            cv.notify_all();
        });
        std::unique_lock<std::mutex> lock(m);
        check(cv.wait_for(lock, std::chrono::seconds(5), [&] { return order.size() == 6; }),
              "the worker runs every task");
    }
    check(order == std::vector<int>({0, 1, 2, 3, 4, 99}), "tasks run in order, past a throwing one");
    check(workerThread != std::this_thread::get_id(), "tasks run on the worker thread");

    // Destroying the worker drops what is still queued.
    std::atomic<int> ran{0};
    std::atomic<bool> started{false}, release{false};
    std::thread releaser;
    {
        BackgroundWorker worker;
        worker.post([&] { started = true; while (!release) std::this_thread::yield(); ++ran; });
        for (int i = 0; i < 10; ++i) worker.post([&] { ++ran; });
        while (!started) std::this_thread::yield();
        // Lets the running task finish while the destructor is waiting for it.
        releaser = std::thread([&] {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            release = true;
        });
    }
    releaser.join();
    check(ran == 1, "the running task finishes; queued ones are dropped on destruction");
}

} // namespace

int main() {
    testServiceManager();
    testSession();
    testCoordinator();
    testBackgroundResolution();
    testResolutionStore();
    testAvailability();
    testBackgroundWorker();
    if (failures == 0) std::cout << "service test passed\n";
    return failures == 0 ? 0 : 1;
}
