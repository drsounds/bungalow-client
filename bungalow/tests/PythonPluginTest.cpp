// Python plugins: the embedded interpreter (PythonHost) turns a .py file into a
// plugin whose services behave like a C++ plugin's. Loads the real sample
// plugin, plus small ones written to a temporary folder for the error cases.

#include "plugin/PluginManager.h"
#include "plugin/PythonHost.h"
#include "service/BackgroundWorker.h"
#include "service/MusicService.h"
#include "service/ServiceManager.h"
#include "service/SessionCipher.h"

#include <nlohmann/json.hpp>

#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

using namespace space;
using namespace space::plugin;
using namespace space::service;
namespace fs = std::filesystem;

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
    void onTrackChanged(const std::string& title, const std::string& artist, const std::string& album) override {
        events.push_back("track:" + title + "/" + artist + "/" + album);
    }
    void onError(const std::string& message) override { events.push_back("error:" + message); }
};

void write(const fs::path& file, const std::string& text) {
    std::ofstream(file, std::ios::binary) << text;
}

void testSamplePlugin(PythonHost& python) {
    UiQueue ui;
    ServiceManager services;
    services.startAll();
    PluginManager plugins(services, ui.executor());

    check(python.loadDirectory(BUNGALOW_TEST_PYTHON_PLUGIN_DIR, plugins, PluginSource::External) == 1,
          "the sample plugin loads from its folder");
    const std::vector<PluginInfo> infos = plugins.plugins();
    check(infos.size() == 1 && infos[0].id == "se.bungalow.pysample" && infos[0].name == "Python Sample Plugin" &&
              infos[0].version == "1.0.0" && infos[0].source == PluginSource::External &&
              infos[0].path.filename() == "sample_plugin.py",
          "its identity comes from ID, NAME and VERSION");

    auto* music = dynamic_cast<MusicService*>(services.get("pysample.stream"));
    check(music != nullptr, "its service is a MusicService");
    if (!music) return;
    check(music->getName() == "Python Sample Stream" && music->getIconPath().empty(), "name and (no) icon");

    // Sign-in.
    check(music->supportsLogin() && !music->isLoggedIn() && !music->getSession().isValid(), "it has accounts");
    std::string result;
    services.login("pysample.stream", [&](const SPSession& s) { result = "ok:" + s.user().name; },
                   [&](const std::string& e) { result = "err:" + e; });
    check(result.empty(), "the outcome waits for the UI thread");
    ui.pump();
    check(result == "ok:Python User" && music->isLoggedIn(), "then the sign-in succeeds");
    auto stored = services.accounts().get("pysample.stream");
    check(stored && stored->user().id == "u7" && stored->encryptedData().find("py-secret") == std::string::npos &&
              stored->data()["token"] == "py-secret",
          "and the session is stored, its data encrypted");
    check(music->getSession().user().name == "Python User", "the service reports its session");
    services.logout("pysample.stream");
    check(!music->isLoggedIn() && !services.accounts().isSignedIn("pysample.stream"), "logout signs out");

    // Lookups come from the background worker's thread.
    std::optional<MusicService::Candidate> found, missing, byTitle;
    std::thread worker([&] {
        found = music->lookup("PYSAMPLE00001");
        missing = music->lookup("NOPE");
        byTitle = music->lookupByTitleArtist("Python Song", "");
    });
    worker.join();
    check(found && found->uri == "pysample:track:1" && found->title == "Python Song" &&
              found->artist == "Python Artist" && found->byIsrc,
          "lookup by ISRC, from another thread");
    check(!missing, "an unknown ISRC finds nothing");
    check(byTitle && byTitle->uri == "pysample:track:1" && !byTitle->byIsrc, "a lookup may return just a URI");

    // Playback.
    check(music->acceptsUri("pysample:track:1") && !music->acceptsUri("file:///x.mp3"), "acceptsUri");
    Recorder recorder;
    music->addPlaybackListener(&recorder);
    music->loadUri("pysample:track:1");
    music->play();
    check(recorder.events.empty(), "playback events wait for the UI thread");
    ui.pump();
    const std::vector<std::string> expected = {"state:1", "track:Python Song/Python Artist/Python Album", "state:2"};
    check(recorder.events == expected, "then arrive in order");
    music->seek(4200);
    check(music->getPositionMs() == 4200 && music->getDurationMs() == 180000, "position and duration");
    music->loadUri("pysample:nope");
    ui.pump();
    check(recorder.events.back() == "error:No such sample track", "errors are reported");
    music->removePlaybackListener(&recorder);

    check(plugins.setEnabled("se.bungalow.pysample", false) && !services.get("pysample.stream"),
          "disabling removes the service");
    check(plugins.setEnabled("se.bungalow.pysample", true) && services.get("pysample.stream"),
          "and enabling registers a new one");
}

void testOtherPlugins(PythonHost& python) {
    const fs::path dir = fs::temp_directory_path() / "bungalow-python-plugin-test";
    std::error_code ec;
    fs::remove_all(dir, ec);
    fs::create_directories(dir / "packaged");
    fs::create_directories(dir / "not_a_plugin");

    // No ID/NAME/VERSION, and a service that only has an identity.
    write(dir / "minimal.py",
          "import bungalow\n"
          "import json, socket, ssl  # the standard library, compiled modules included\n"
          "class Plain(bungalow.Service):\n"
          "    id, name, icon = 'plain.service', 'Plain', 'icons/Plain.svg'\n"
          "def activate(context):\n"
          "    assert context.id == 'py.minimal'\n"
          "    context.register_service(Plain())\n");
    // A package, whose helper module is importable relative to it.
    write(dir / "packaged" / "helper.py", "SERVICE_ID = 'packaged.service'\n");
    write(dir / "packaged" / "__init__.py",
          "import bungalow\n"
          "from .helper import SERVICE_ID\n"
          "ID = 'com.test.packaged'\n"
          "class Packaged(bungalow.Service):\n"
          "    id = SERVICE_ID\n"
          "    def login(self, done): done.error('wrong password')\n"
          "def activate(context):\n"
          "    context.register_service(Packaged())\n");
    // Broken ones: none may take the others (or the host) down.
    write(dir / "broken_syntax.py", "def activate(context:\n");
    write(dir / "no_activate.py", "import bungalow\n");
    write(dir / "raises.py",
          "import bungalow\n"
          "class Early(bungalow.Service):\n"
          "    id = 'early.service'\n"
          "def activate(context):\n"
          "    context.register_service(Early())\n"
          "    raise RuntimeError('activation failed')\n");
    write(dir / "taken_id.py",
          "import bungalow\n"
          "class Clash(bungalow.Service):\n"
          "    id = 'plain.service'\n"
          "def activate(context):\n"
          "    context.register_service(Clash())\n");
    write(dir / "_private.py", "raise RuntimeError('must not be imported')\n");
    write(dir / "not_a_plugin" / "readme.txt", "no __init__.py here\n");

    UiQueue ui;
    ServiceManager services;
    services.startAll();
    PluginManager plugins(services, ui.executor());

    std::cerr << "(the errors reported below are the broken plugins, on purpose)\n";
    check(python.loadDirectory(dir, plugins, PluginSource::External) == 2, "only the two valid plugins load");

    std::vector<std::string> ids;
    for (const PluginInfo& info : plugins.plugins()) ids.push_back(info.id);
    check(ids == std::vector<std::string>{"py.minimal", "com.test.packaged"}, "ids default to py.<file name>");

    Service* plain = services.get("plain.service");
    check(plain && plain->getIconPath() == "icons/Plain.svg" && !plain->supportsLogin() &&
              dynamic_cast<MediaService*>(plain) == nullptr,
          "a service gets only the capabilities its class overrides");
    check(!services.get("early.service"), "a failed activation is rolled back");

    std::string loginError;
    services.login("packaged.service", nullptr, [&](const std::string& e) { loginError = e; });
    ui.pump();
    check(loginError == "wrong password" && !services.accounts().isSignedIn("packaged.service"),
          "done.error() reports a failed sign-in");

    std::string error;
    check(!python.loadFile(dir / "broken_syntax.py", plugins, PluginSource::External, &error) &&
              error.find("SyntaxError") != std::string::npos,
          "a syntax error is reported as such");
    check(!python.loadFile(dir / "no_activate.py", plugins, PluginSource::External, &error) &&
              error.find("activate") != std::string::npos,
          "a module without activate() is not a plugin");

    plugins.unloadAll();
    check(services.all().empty(), "unloading leaves no service");
    fs::remove_all(dir, ec);
}

} // namespace

int main() {
    check(PythonHost::available(), "this build embeds Python");

    std::string error;
    check(PythonHost::create(fs::path(BUNGALOW_TEST_PYTHON_HOME) / "missing", &error) == nullptr && !error.empty(),
          "a missing standard library is an error, not a crash");

    SessionCipher::setInstance(std::make_shared<XorCipher>());
    std::unique_ptr<PythonHost> python = PythonHost::create(BUNGALOW_TEST_PYTHON_HOME, &error);
    check(python != nullptr, "the interpreter starts: " + error);
    if (python) {
        testSamplePlugin(*python);
        testOtherPlugins(*python);
        python.reset();
    }
    SessionCipher::setInstance(nullptr);

    if (failures == 0) std::cout << "python plugin test passed\n";
    return failures == 0 ? 0 : 1;
}
