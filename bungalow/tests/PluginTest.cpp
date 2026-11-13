// The plugin architecture: COM objects presented as native services
// (ComServiceAdapter), and plugins loaded from libraries and activated through
// a recording context (PluginManager). Loads the real sample plugin library.

#include "plugin/ComServiceAdapter.h"
#include "plugin/PluginManager.h"
#include "service/MusicService.h"
#include "service/ServiceManager.h"
#include "service/SessionCipher.h"

#include <nlohmann/json.hpp>

#include <deque>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

using namespace space;
using namespace space::com;
using namespace space::plugin;
using namespace space::service;

namespace {

int failures = 0;

void check(bool condition, const std::string& what) {
    if (condition) return;
    ++failures;
    std::cerr << "FAILED: " << what << "\n";
}

/// Stands in for the UI thread: tasks wait until the test pumps them.
struct UiQueue {
    std::deque<std::function<void()>> tasks;
    Executor executor() {
        return [this](std::function<void()> task) { tasks.push_back(std::move(task)); };
    }
    void pump() {
        while (!tasks.empty()) {
            auto task = std::move(tasks.front());
            tasks.pop_front();
            task();
        }
    }
};

class XorCipher : public SessionCipher {
public:
    bool encrypt(const std::string& in, std::string& out) override { out = flip(in); return true; }
    bool decrypt(const std::string& in, std::string& out) override { out = flip(in); return true; }
private:
    static std::string flip(std::string s) { for (char& c : s) c ^= 0x5a; return s; }
};

struct Recorder : MediaService::PlaybackListener {
    std::vector<std::string> events;
    void onStateChanged(MediaService::PlaybackState state) override {
        events.push_back("state:" + std::to_string(static_cast<int>(state)));
    }
    void onTrackChanged(const std::string& title, const std::string& artist, const std::string&) override {
        events.push_back("track:" + title + "/" + artist);
    }
    void onError(const std::string& message) override { events.push_back("error:" + message); }
};

// ── In-process plugins, for the cases the sample library does not cover ──────

int g_liveServices = 0;

/// Identity and lifecycle only: no sign-in, no playback.
class PlainService final : public ComObject<IService> {
public:
    PlainService(std::string id, std::vector<std::string>* log) : m_id(std::move(id)), m_log(log) { ++g_liveServices; }
    ~PlainService() override { --g_liveServices; }

    Result BUNGALOW_CALL GetId(IString** out) override { return returnString(m_id, out); }
    Result BUNGALOW_CALL GetName(IString** out) override { return returnString("Plain " + m_id, out); }
    Result BUNGALOW_CALL GetIconPath(IString** out) override { return returnString("icons/Plain.svg", out); }
    Result BUNGALOW_CALL OnCreate() override { return log("create"); }
    Result BUNGALOW_CALL OnStart() override { return log("start"); }
    Result BUNGALOW_CALL OnStop() override { return log("stop"); }
    Result BUNGALOW_CALL OnDestroy() override { return log("destroy"); }

private:
    Result log(const std::string& what) {
        m_log->push_back(m_id + ":" + what);
        return kOk;
    }
    std::string               m_id;
    std::vector<std::string>* m_log;
};

class FakeApplication final : public ComObject<IApplication> {
public:
    FakeApplication(std::string id, std::vector<std::string> serviceIds, std::vector<std::string>* log,
                    Result activateResult = kOk)
        : m_id(std::move(id)), m_serviceIds(std::move(serviceIds)), m_log(log), m_activateResult(activateResult) {}

    Result BUNGALOW_CALL GetId(IString** out) override { return returnString(m_id, out); }
    Result BUNGALOW_CALL GetName(IString** out) override { return returnString("Fake", out); }
    Result BUNGALOW_CALL GetVersion(IString** out) override { return returnString("2.1.0", out); }

    Result BUNGALOW_CALL OnActivate(IApplicationContext* context) override {
        m_log->push_back(m_id + ":activate");
        featureResult = context->RegisterFeature(nullptr);
        contextId = getString([&](IString** o) { return context->GetApplicationId(o); });
        for (const std::string& id : m_serviceIds) {
            auto service = make<PlainService>(id, m_log);
            registerResults.push_back(context->RegisterService(service.get()));
        }
        keptContext = ComPtr<IApplicationContext>(context); // against the rules, to prove it is closed
        return m_activateResult;
    }
    Result BUNGALOW_CALL OnDeactivate() override {
        m_log->push_back(m_id + ":deactivate");
        return kOk;
    }

    std::vector<Result>          registerResults;
    Result                       featureResult = kOk;
    std::string                  contextId;
    ComPtr<IApplicationContext>  keptContext;

private:
    std::string               m_id;
    std::vector<std::string>  m_serviceIds;
    std::vector<std::string>* m_log;
    Result                    m_activateResult;
};

// ── Tests ────────────────────────────────────────────────────────────────────

void testComObject() {
    std::vector<std::string> log;
    auto service = make<PlainService>("x", &log);
    ComPtr<IAspect> aspect = service.as<IAspect>();
    ComPtr<IUnknown> unknown = service.as<IUnknown>();
    check(aspect && unknown, "QueryInterface answers base interfaces and IUnknown");
    check(!service.as<IAuthAspect>() && !service.as<IMediaService>(), "and refuses what is not implemented");
    check(getString([&](IString** o) { return aspect->GetId(o); }) == "x", "strings cross as IString");
    aspect.reset();
    unknown.reset();
    check(g_liveServices == 1, "references are counted");
    service.reset();
    check(g_liveServices == 0, "the last Release destroys the object");
}

void testManagerInProcess() {
    std::vector<std::string> log;
    UiQueue ui;
    {
        ServiceManager services;
        services.startAll();
        PluginManager plugins(services, ui.executor());

        auto app = make<FakeApplication>("com.test.fake", std::vector<std::string>{"fake.a", "fake.a", "fake.b"}, &log);
        std::string error;
        check(plugins.addApplication(ComPtr<IApplication>(app.get()), &error), "an application activates: " + error);

        check(app->contextId == "com.test.fake", "the context knows the plugin id");
        check(app->featureResult == kNotImpl, "features are not accepted yet");
        check(app->registerResults.size() == 3 && app->registerResults[0] == kOk &&
                  failed(app->registerResults[1]) && app->registerResults[2] == kOk,
              "a service id can be registered only once");
        check(failed(app->keptContext->RegisterService(nullptr)) &&
                  failed(app->keptContext->GetApplicationId(nullptr)),
              "the context is closed after OnActivate");

        Service* a = services.get("fake.a");
        check(a && services.get("fake.b"), "its services are in the ServiceManager");
        check(a && a->getName() == "Plain fake.a" && a->getIconPath() == "icons/Plain.svg",
              "identity comes from the COM object");
        check(a && !a->supportsLogin() && !a->isLoggedIn() && !a->getSession().isValid(),
              "a service without IAuthAspect has no sign-in");
        check(a && dynamic_cast<MediaService*>(a) == nullptr, "nor playback without IMediaService");

        std::string loginError;
        services.login("fake.a", nullptr, [&](const std::string& e) { loginError = e; });
        check(loginError.empty(), "sign-in outcomes arrive on the UI thread");
        ui.pump();
        check(!loginError.empty(), "signing in to a service without accounts fails");

        const std::vector<PluginInfo> infos = plugins.plugins();
        check(infos.size() == 1 && infos[0].active && infos[0].version == "2.1.0" &&
                  infos[0].source == PluginSource::Builtin && infos[0].serviceIds.size() == 2,
              "the manager describes the plugin");

        check(!plugins.addApplication(make<FakeApplication>("com.test.fake", std::vector<std::string>{}, &log).as<IApplication>()),
              "a plugin id loads only once");

        check(plugins.setEnabled("com.test.fake", false), "a plugin can be disabled");
        check(!services.get("fake.a") && !services.get("fake.b") && g_liveServices == 0,
              "disabling removes and releases its services");
        check(!plugins.plugins()[0].active, "and marks it inactive");
        check(plugins.setEnabled("com.test.fake", true) && services.get("fake.a"), "and enabled again");
        check(!plugins.setEnabled("nope", true), "unknown plugins cannot be enabled");

        // A plugin whose activation fails leaves nothing behind.
        auto bad = make<FakeApplication>("com.test.bad", std::vector<std::string>{"bad.a"}, &log, kFail);
        check(!plugins.addApplication(bad.as<IApplication>()), "a failing activation is reported");
        check(!services.get("bad.a") && plugins.plugins().size() == 1, "and rolled back");
    }
    check(g_liveServices == 0, "destroying the manager releases everything");

    const std::vector<std::string> expected = {
        "com.test.fake:activate", "fake.a:create", "fake.a:start", "fake.b:create", "fake.b:start",
        "fake.b:stop", "fake.b:destroy", "fake.a:stop", "fake.a:destroy", "com.test.fake:deactivate",
        "com.test.fake:activate", "fake.a:create", "fake.a:start", "fake.b:create", "fake.b:start",
        "com.test.bad:activate", "bad.a:create", "bad.a:start", "bad.a:stop", "bad.a:destroy", "com.test.bad:deactivate",
        "fake.b:stop", "fake.b:destroy", "fake.a:stop", "fake.a:destroy", "com.test.fake:deactivate",
    };
    check(log == expected, "lifecycle order");
    if (log != expected)
        for (const std::string& line : log) std::cerr << "  " << line << "\n";
}

void testSampleLibrary() {
    SessionCipher::setInstance(std::make_shared<XorCipher>());
    UiQueue ui;
    ServiceManager services;
    services.startAll();
    {
        PluginManager plugins(services, ui.executor());
        const std::filesystem::path dir = BUNGALOW_TEST_PLUGIN_DIR;
        check(plugins.loadDirectory(dir, PluginSource::External) == 1, "the sample library loads from its folder");
        check(plugins.loadDirectory(dir / "missing", PluginSource::External) == 0, "a missing folder is no error");

        const std::vector<PluginInfo> infos = plugins.plugins();
        check(infos.size() == 1 && infos[0].id == "se.bungalow.sample" && infos[0].name == "Sample Plugin" &&
                  infos[0].version == "1.0.0" && infos[0].source == PluginSource::External &&
                  !infos[0].path.empty(),
              "its identity comes from the IApplication");
        if (infos.empty()) return;

        std::string error;
        check(!plugins.loadLibrary(infos[0].path, PluginSource::External, &error) && !error.empty(),
              "loading it twice is refused");

        auto* music = dynamic_cast<MusicService*>(services.get("sample.stream"));
        check(music != nullptr, "its service is a MusicService");
        if (!music) return;
        check(services.getServices<MusicService>().size() == 1, "found by capability");

        // Sign-in.
        check(music->supportsLogin() && !music->isLoggedIn(), "it has accounts");
        std::string result;
        services.login("sample.stream", [&](const SPSession& s) { result = "ok:" + s.user().name; },
                       [&](const std::string& e) { result = "err:" + e; });
        check(result.empty() && !services.accounts().isSignedIn("sample.stream"),
              "the outcome waits for the UI thread");
        ui.pump();
        check(result == "ok:Sample User" && music->isLoggedIn(), "then the sign-in succeeds");
        auto stored = services.accounts().get("sample.stream");
        check(stored && stored->user().id == "u42" && stored->serviceId() == "sample.stream",
              "and the session is stored");
        check(stored && stored->encryptedData().find("abc123") == std::string::npos &&
                  stored->data()["token"] == "abc123",
              "with its data encrypted");
        check(music->getSession().user().name == "Sample User" && music->getAccountName() == "Sample User",
              "the service reports its session");

        // Resolution and playback.
        auto found = music->lookup("SAMPLE0000001");
        check(found && found->uri == "sample:track:1" && found->title == "Sample Song" && found->byIsrc,
              "lookup by ISRC");
        check(!music->lookup("NOPE"), "an unknown ISRC finds nothing");
        auto byTitle = music->lookupByTitleArtist("Sample Song", "");
        check(byTitle && !byTitle->byIsrc, "lookup by title");
        check(music->acceptsUri("sample:track:1") && !music->acceptsUri("file:///x.mp3"), "acceptsUri");

        Recorder recorder;
        music->addPlaybackListener(&recorder);
        music->loadUri("sample:track:1");
        music->play();
        check(recorder.events.empty(), "playback events wait for the UI thread");
        ui.pump();
        const std::vector<std::string> expected = {"state:1", "track:Sample Song/Sample Artist", "state:2"};
        check(recorder.events == expected, "then arrive in order");
        check(music->getPlaybackState() == MediaService::PlaybackState::Playing && music->getDurationMs() == 180000,
              "state and duration");
        music->loadUri("sample:nope");
        ui.pump();
        check(recorder.events.back() == "error:No such sample track" &&
                  music->getPlaybackState() == MediaService::PlaybackState::Error,
              "errors are reported");
        music->removePlaybackListener(&recorder);

        // Events raised just before the service goes away are dropped, not delivered to a dead object.
        music->stop();
        check(plugins.setEnabled("se.bungalow.sample", false), "the plugin can be disabled");
        check(!services.get("sample.stream") && !services.accounts().isSignedIn("sample.stream"),
              "which removes the service and its account");
        ui.pump();
    }
    check(services.all().empty(), "unloading leaves no service");
    SessionCipher::setInstance(nullptr);
}

} // namespace

int main() {
    testComObject();
    testManagerInProcess();
    testSampleLibrary();
    if (failures == 0) std::cout << "plugin test passed\n";
    return failures == 0 ? 0 : 1;
}
