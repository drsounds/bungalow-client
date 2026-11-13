#include "plugin/PythonHost.h"

#ifndef BUNGALOW_PYTHON

// ── Built without Python ─────────────────────────────────────────────────────

namespace space {
namespace plugin {

struct PythonHost::Impl {};

bool PythonHost::available() { return false; }

std::unique_ptr<PythonHost> PythonHost::create(const std::filesystem::path&, std::string* error) {
    if (error) *error = "This build of Bungalow has no Python support";
    return nullptr;
}

PythonHost::PythonHost(std::unique_ptr<Impl> impl) : m_impl(std::move(impl)) {}
PythonHost::~PythonHost() = default;

int PythonHost::loadDirectory(const std::filesystem::path&, PluginManager&, PluginSource) { return 0; }

bool PythonHost::loadFile(const std::filesystem::path&, PluginManager&, PluginSource, std::string* error) {
    if (error) *error = "This build of Bungalow has no Python support";
    return false;
}

} // namespace plugin
} // namespace space

#else

// ── Embedded Python ──────────────────────────────────────────────────────────
//
// A Python plugin is turned into the same COM objects a C++ plugin provides:
// PyApplication (com::IApplication) around its module, PyService
// (com::IService + capabilities) around each bungalow.Service it registers.
// From there the PluginManager and the adapters treat it like any other.
//
// The Python side of the API (the `bungalow` module) is the source in
// kBungalowModule; it does the marshalling, so the C++ side only exchanges
// strings, numbers and tuples with the `_host_*` methods of bungalow.Service.
// The native bits it needs (reporting a sign-in, playback events, registering
// a service) are in the `_bungalow` module.
//
// The GIL is released while the host runs; every entry into Python takes it.

#include "plugin/PluginSupport.h"

#include <pybind11/embed.h>
#include <pybind11/stl.h>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <mutex>
#include <optional>

namespace py = pybind11;

namespace space {
namespace plugin {

using namespace com;
namespace fs = std::filesystem;

namespace {

const char* const kBungalowModule = R"PY(
"""The Bungalow plugin API. See docs/docs/architecture/plugins.md."""
import importlib.util as _util
import json as _json
import os as _os
import sys as _sys

# Playback states, for Service.playback.state().
IDLE, LOADING, PLAYING, PAUSED, STOPPED, ERROR = range(6)


class Session:
    """A sign-in: who is signed in, plus what is needed to resume it.

    `data` is anything json.dumps accepts (tokens, ...); Bungalow encrypts it.
    """

    def __init__(self, user_id, user_name="", data=None):
        self.user_id = user_id
        self.user_name = user_name
        self.data = data


class Track:
    """Something a service can play: a URI its load_uri() accepts."""

    def __init__(self, uri, title="", artist=""):
        self.uri = uri
        self.title = title
        self.artist = artist


class _NoPlayback:
    def state(self, state): pass
    def position(self, position_ms, duration_ms): pass
    def track(self, title, artist="", album=""): pass
    def error(self, message): pass
    def completed(self): pass


class Service:
    """Base class of a service. Override what the service can do:

    - login()                          -> it has accounts
    - load_uri() (and play(), ...)     -> it plays media
    - lookup() / lookup_by_title_artist(), with load_uri() -> it plays music
    """

    id = ""            # stable and unique, e.g. "acme.stream"
    name = ""
    icon = ""          # path of an icon, optional
    supports_login = None   # None: yes exactly when login() is overridden

    #: Report playback here: state(), position(), track(), error(), completed().
    #: Safe to call from any thread.
    playback = _NoPlayback()

    _session = None

    # Lifecycle
    def on_create(self): pass
    def on_start(self): pass
    def on_stop(self): pass
    def on_destroy(self): pass

    # Sign-in. login() returns at once; call done.success(Session(...)) or
    # done.error("why") when the attempt finishes, from any thread.
    def login(self, done): done.error("This service has no sign-in")
    def logout(self): pass
    def is_logged_in(self): return self._session is not None
    def session(self): return self._session

    # Playback
    def accepts_uri(self, uri): return False
    def load_uri(self, uri): raise NotImplementedError
    def play(self): pass
    def pause(self): pass
    def stop(self): pass
    def seek(self, position_ms): pass
    def position_ms(self): return 0
    def duration_ms(self): return 0

    # Music: return a Track (or just its URI), or None. Called on a
    # background thread.
    def lookup(self, isrc): return None
    def lookup_by_title_artist(self, title, artist): return None

    # -- Called by the host ---------------------------------------------------
    def _host_caps(self):
        cls, caps = type(self), 0
        if cls.login is not Service.login: caps |= 1
        if cls.load_uri is not Service.load_uri: caps |= 2
        if (cls.lookup is not Service.lookup
                or cls.lookup_by_title_artist is not Service.lookup_by_title_artist): caps |= 4
        return caps

    def _host_supports_login(self):
        if self.supports_login is None: return bool(self._host_caps() & 1)
        return bool(self.supports_login)

    def _host_login(self, native): self.login(_Login(self, native))

    def _host_logout(self):
        try:
            self.logout()
        finally:
            self._session = None

    def _host_session(self):
        return _pack_session(self.session() if self.is_logged_in() else None)

    def _host_lookup(self, isrc): return _pack_track(self.lookup(isrc))

    def _host_lookup_by_title_artist(self, title, artist):
        return _pack_track(self.lookup_by_title_artist(title, artist))


class _Login:
    def __init__(self, service, native):
        self._service, self._native = service, native

    def success(self, session):
        packed = _pack_session(session)
        if packed is None: raise TypeError("success() takes a bungalow.Session")
        self._service._session = session
        self._native.success(*packed)

    def error(self, message):
        self._native.error(str(message))


def _pack_session(session):
    if session is None: return None
    if not isinstance(session, Session): raise TypeError("expected a bungalow.Session")
    data = None if session.data is None else _json.dumps(session.data)
    return (str(session.user_id), str(session.user_name), data)


def _pack_track(track):
    if track is None: return None
    if isinstance(track, str): return (track, "", "")
    if isinstance(track, Track): return (str(track.uri), str(track.title), str(track.artist))
    raise TypeError("expected a bungalow.Track, a URI or None")


def _load(path, module_name):
    """Imports the plugin at `path` (a .py file or a package folder)."""
    package = _os.path.isdir(path)
    source = _os.path.join(path, "__init__.py") if package else path
    spec = _util.spec_from_file_location(
        module_name, source, submodule_search_locations=[path] if package else None)
    module = _util.module_from_spec(spec)
    _sys.modules[module_name] = module
    try:
        spec.loader.exec_module(module)
        if not callable(getattr(module, "activate", None)):
            raise TypeError("a Bungalow plugin defines activate(context)")
    except BaseException:
        _sys.modules.pop(module_name, None)
        raise
    return module
)PY";

constexpr int kCapAuth = 1;
constexpr int kCapMedia = 2;
constexpr int kCapMusic = 4;

void report(const std::string& where, const std::string& what) {
    std::cerr << "[python plugin] " << where << ": " << what << std::endl;
}

/// Runs `fn` (which returns a Result) with the GIL held; a Python exception
/// becomes kFail and is written to stderr.
template <class Fn> Result guarded(const char* where, Fn&& fn) {
    py::gil_scoped_acquire gil;
    try {
        return fn();
    } catch (py::error_already_set& e) {
        report(where, e.what());
    } catch (const std::exception& e) {
        report(where, e.what());
    }
    return kFail;
}

std::string text(const py::handle& value) {
    return value.is_none() ? std::string() : std::string(py::str(value));
}

// ── Native objects handed to Python (module _bungalow) ───────────────────────

/// A session as plain data, so the host can read it without entering Python.
class SnapshotSession final : public ComObject<ISession> {
public:
    SnapshotSession(std::string id, std::string name, std::optional<std::string> data)
        : m_id(std::move(id)), m_name(std::move(name)), m_data(std::move(data)) {}

    Result BUNGALOW_CALL GetUserId(IString** out) override { return returnString(m_id, out); }
    Result BUNGALOW_CALL GetUserDisplayName(IString** out) override { return returnString(m_name, out); }
    Result BUNGALOW_CALL GetData(IString** out) override {
        if (!out) return kPointer;
        *out = nullptr;
        return m_data ? returnString(*m_data, out) : kFalse;
    }

private:
    std::string                m_id;
    std::string                m_name;
    std::optional<std::string> m_data;
};

struct NativeLogin {
    ComPtr<ILoginCallback> callback;

    void success(const std::string& id, const std::string& name, std::optional<std::string> data) {
        auto session = make<SnapshotSession>(id, name, std::move(data));
        callback->OnSuccess(session.get());
    }
    void error(const std::string& message) { callback->OnError(message.c_str()); }
};

struct NativePlayback {
    std::mutex                mutex;
    ComPtr<IPlaybackListener> listener;

    ComPtr<IPlaybackListener> get() {
        std::lock_guard<std::mutex> lock(mutex);
        return listener;
    }
    void set(IPlaybackListener* l) {
        std::lock_guard<std::mutex> lock(mutex);
        listener = ComPtr<IPlaybackListener>(l);
    }
    void state(int value) {
        if (auto l = get()) l->OnStateChanged(value);
    }
    void position(int positionMs, int durationMs) {
        if (auto l = get()) l->OnPositionChanged(positionMs, durationMs);
    }
    void track(const std::string& title, const std::string& artist, const std::string& album) {
        if (auto l = get()) l->OnTrackChanged(title.c_str(), artist.c_str(), album.c_str());
    }
    void error(const std::string& message) {
        if (auto l = get()) l->OnError(message.c_str());
    }
    void completed() {
        if (auto l = get()) l->OnCompleted();
    }
};

struct NativeContext {
    IApplicationContext* context = nullptr; // null once activate() returned
    std::string          id;

    void registerService(py::object service);
};

// ── PyService ────────────────────────────────────────────────────────────────

class PyService final : public ComObject<IService, IAuthAspect, IMediaService, IMusicService> {
public:
    /// GIL held.
    PyService(py::object object, int caps)
        : m_object(std::move(object)), m_caps(caps), m_playback(std::make_shared<NativePlayback>()) {
        if (m_caps & kCapMedia) m_object.attr("playback") = py::cast(m_playback);
    }

    ~PyService() override {
        if (!Py_IsInitialized()) {
            m_object.release(); // the interpreter is gone, and the object with it
            return;
        }
        py::gil_scoped_acquire gil;
        m_object = py::object();
    }

    /// Answers only the capabilities the Python class overrides.
    Result BUNGALOW_CALL QueryInterface(const Guid& iid, void** out) override {
        const bool refused = (iid == IID_IAuthAspect && !(m_caps & kCapAuth)) ||
                             (iid == IID_IMediaService && !(m_caps & kCapMedia)) ||
                             (iid == IID_IMusicService && !(m_caps & kCapMusic));
        if (refused) {
            if (out) *out = nullptr;
            return kNoInterface;
        }
        return ComObject::QueryInterface(iid, out);
    }

    // IAspect / IService
    Result BUNGALOW_CALL GetId(IString** out) override { return attribute("id", out); }
    Result BUNGALOW_CALL GetName(IString** out) override { return attribute("name", out); }
    Result BUNGALOW_CALL GetIconPath(IString** out) override { return attribute("icon", out); }
    Result BUNGALOW_CALL OnCreate() override { return call("on_create"); }
    Result BUNGALOW_CALL OnStart() override { return call("on_start"); }
    Result BUNGALOW_CALL OnStop() override { return call("on_stop"); }
    Result BUNGALOW_CALL OnDestroy() override { return call("on_destroy"); }

    // IAuthAspect
    Result BUNGALOW_CALL SupportsLogin(int32_t* out) override { return boolean("_host_supports_login", out); }
    Result BUNGALOW_CALL Login(ILoginCallback* callback) override {
        if (!callback) return kPointer;
        return guarded("login", [&] {
            auto native = std::make_shared<NativeLogin>();
            native->callback = ComPtr<ILoginCallback>(callback);
            m_object.attr("_host_login")(native);
            return kOk;
        });
    }
    Result BUNGALOW_CALL Logout() override { return call("_host_logout"); }
    Result BUNGALOW_CALL IsLoggedIn(int32_t* out) override { return boolean("is_logged_in", out); }
    Result BUNGALOW_CALL GetSession(ISession** out) override {
        if (!out) return kPointer;
        *out = nullptr;
        return guarded("session", [&] {
            py::object packed = m_object.attr("_host_session")();
            if (packed.is_none()) return kFalse;
            auto t = packed.cast<std::tuple<std::string, std::string, std::optional<std::string>>>();
            *out = new SnapshotSession(std::get<0>(t), std::get<1>(t), std::get<2>(t));
            return kOk;
        });
    }

    // IMediaService
    Result BUNGALOW_CALL SetPlaybackListener(IPlaybackListener* listener) override {
        m_playback->set(listener);
        return kOk;
    }
    Result BUNGALOW_CALL AcceptsUri(const char* uri, int32_t* out) override {
        if (!uri || !out) return kPointer;
        return guarded("accepts_uri", [&] {
            *out = m_object.attr("accepts_uri")(uri).cast<bool>() ? 1 : 0;
            return kOk;
        });
    }
    Result BUNGALOW_CALL LoadUri(const char* uri) override {
        if (!uri) return kPointer;
        return guarded("load_uri", [&] {
            m_object.attr("load_uri")(uri);
            return kOk;
        });
    }
    Result BUNGALOW_CALL Play() override { return call("play"); }
    Result BUNGALOW_CALL Pause() override { return call("pause"); }
    Result BUNGALOW_CALL Stop() override { return call("stop"); }
    Result BUNGALOW_CALL Seek(int32_t positionMs) override {
        return guarded("seek", [&] {
            m_object.attr("seek")(positionMs);
            return kOk;
        });
    }
    Result BUNGALOW_CALL GetPositionMs(int32_t* out) override { return integer("position_ms", out); }
    Result BUNGALOW_CALL GetDurationMs(int32_t* out) override { return integer("duration_ms", out); }

    // IMusicService
    Result BUNGALOW_CALL Lookup(const char* isrc, IString** outUri, IString** outTitle,
                                IString** outArtist) override {
        if (!isrc) return kPointer;
        return guarded("lookup", [&] {
            return track(m_object.attr("_host_lookup")(isrc), outUri, outTitle, outArtist);
        });
    }
    Result BUNGALOW_CALL LookupByTitleArtist(const char* title, const char* artist, IString** outUri,
                                             IString** outTitle, IString** outArtist) override {
        if (!title) return kPointer;
        return guarded("lookup_by_title_artist", [&] {
            return track(m_object.attr("_host_lookup_by_title_artist")(title, artist ? artist : ""), outUri,
                         outTitle, outArtist);
        });
    }

private:
    Result attribute(const char* name, IString** out) {
        if (!out) return kPointer;
        *out = nullptr;
        return guarded(name, [&] {
            const std::string value = text(m_object.attr(name));
            return value.empty() ? kFalse : returnString(value, out);
        });
    }
    Result call(const char* method) {
        return guarded(method, [&] {
            m_object.attr(method)();
            return kOk;
        });
    }
    Result boolean(const char* method, int32_t* out) {
        if (!out) return kPointer;
        return guarded(method, [&] {
            *out = m_object.attr(method)().cast<bool>() ? 1 : 0;
            return kOk;
        });
    }
    Result integer(const char* method, int32_t* out) {
        if (!out) return kPointer;
        return guarded(method, [&] {
            *out = m_object.attr(method)().cast<int32_t>();
            return kOk;
        });
    }
    static Result track(const py::object& packed, IString** outUri, IString** outTitle, IString** outArtist) {
        if (packed.is_none()) return kFalse;
        auto t = packed.cast<std::tuple<std::string, std::string, std::string>>();
        if (std::get<0>(t).empty()) return kFalse;
        returnString(std::get<0>(t), outUri);
        returnString(std::get<1>(t), outTitle);
        returnString(std::get<2>(t), outArtist);
        return kOk;
    }

    py::object                      m_object;
    int                             m_caps;
    std::shared_ptr<NativePlayback> m_playback;
};

void NativeContext::registerService(py::object service) {
    if (!context) throw std::runtime_error("the context is only valid inside activate()");
    if (!py::hasattr(service, "_host_caps")) throw py::type_error("register_service() takes a bungalow.Service");
    if (text(service.attr("id")).empty()) throw py::value_error("the service has no id");

    const int caps = service.attr("_host_caps")().cast<int>();
    auto wrapped = make<PyService>(std::move(service), caps);
    if (failed(context->RegisterService(wrapped.get())))
        throw std::runtime_error("Bungalow refused the service (is its id already taken?)");
}

// ── PyApplication ────────────────────────────────────────────────────────────

class PyApplication final : public ComObject<IApplication> {
public:
    /// GIL held.
    PyApplication(py::object module, std::string id, std::string name, std::string version)
        : m_module(std::move(module)), m_id(std::move(id)), m_name(std::move(name)),
          m_version(std::move(version)) {}

    ~PyApplication() override {
        if (!Py_IsInitialized()) {
            m_module.release();
            return;
        }
        py::gil_scoped_acquire gil;
        m_module = py::object();
    }

    Result BUNGALOW_CALL GetId(IString** out) override { return returnString(m_id, out); }
    Result BUNGALOW_CALL GetName(IString** out) override { return returnString(m_name, out); }
    Result BUNGALOW_CALL GetVersion(IString** out) override { return returnString(m_version, out); }

    Result BUNGALOW_CALL OnActivate(IApplicationContext* context) override {
        if (!context) return kPointer;
        auto native = std::make_shared<NativeContext>();
        native->context = context;
        native->id = m_id;
        const Result result = guarded("activate", [&] {
            m_module.attr("activate")(native);
            return kOk;
        });
        native->context = nullptr; // a plugin that kept it gets an error
        return result;
    }

    Result BUNGALOW_CALL OnDeactivate() override {
        return guarded("deactivate", [&] {
            if (py::hasattr(m_module, "deactivate")) m_module.attr("deactivate")();
            return kOk;
        });
    }

private:
    py::object  m_module;
    std::string m_id;
    std::string m_name;
    std::string m_version;
};

std::string moduleNameFor(const fs::path& path) {
    std::string name = "bungalow_plugin_" + path.stem().u8string();
    for (char& c : name)
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_')) c = '_';
    return name;
}

bool isPythonPlugin(const fs::path& path) {
    const std::string name = path.filename().u8string();
    if (name.empty() || name[0] == '_' || name[0] == '.') return false;
    std::error_code ec;
    if (fs::is_directory(path, ec)) return fs::exists(path / "__init__.py", ec);
    std::string ext = path.extension().u8string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(::tolower(c)); });
    return ext == ".py";
}

} // namespace

PYBIND11_EMBEDDED_MODULE(_bungalow, m) {
    py::class_<NativeLogin, std::shared_ptr<NativeLogin>>(m, "Login")
        .def("success", &NativeLogin::success)
        .def("error", &NativeLogin::error);

    py::class_<NativePlayback, std::shared_ptr<NativePlayback>>(m, "Playback")
        .def("state", &NativePlayback::state)
        .def("position", &NativePlayback::position)
        .def("track", &NativePlayback::track, py::arg("title"), py::arg("artist") = "", py::arg("album") = "")
        .def("error", &NativePlayback::error)
        .def("completed", &NativePlayback::completed);

    py::class_<NativeContext, std::shared_ptr<NativeContext>>(m, "Context")
        .def_readonly("id", &NativeContext::id)
        .def("register_service", &NativeContext::registerService);
}

// ── PythonHost ───────────────────────────────────────────────────────────────

struct PythonHost::Impl {
    // Declared in this order: the interpreter outlives the objects below.
    std::unique_ptr<py::scoped_interpreter>  interpreter;
    py::object                               load;     // bungalow._load
    std::unique_ptr<py::gil_scoped_release>  released; // while the host runs

    ~Impl() {
        released.reset(); // take the GIL back for the teardown
        load = py::object();
    }
};

bool PythonHost::available() { return true; }

std::unique_ptr<PythonHost> PythonHost::create(const fs::path& home, std::string* error) {
    auto fail = [&](const std::string& message) {
        if (error) *error = message;
        return nullptr;
    };
    if (Py_IsInitialized()) return fail("Python is already running");

    std::error_code ec;
    const fs::path lib = home / "Lib";
    if (!fs::exists(lib / "encodings", ec))
        return fail("The Python standard library was not found in " + lib.u8string());

    auto impl = std::make_unique<Impl>();
    try {
        // Isolated: no environment variables, no user site-packages, no registry.
        PyConfig config;
        PyConfig_InitIsolatedConfig(&config);
        PyConfig_SetString(&config, &config.home, home.wstring().c_str());
        config.module_search_paths_set = 1;
        PyWideStringList_Append(&config.module_search_paths, lib.wstring().c_str());
        PyWideStringList_Append(&config.module_search_paths, (lib / "site-packages").wstring().c_str());
        impl->interpreter = std::make_unique<py::scoped_interpreter>(&config, 0, nullptr, false);

        py::module_::import("_bungalow"); // registers the native types passed to Python below

        // The `bungalow` module plugins import.
        py::module_ module = py::reinterpret_borrow<py::module_>(
            py::module_::import("types").attr("ModuleType")("bungalow"));
        py::exec(kBungalowModule, module.attr("__dict__"));
        py::module_::import("sys").attr("modules")["bungalow"] = module;
        impl->load = module.attr("_load");
    } catch (const std::exception& e) {
        const std::string message = e.what();
        impl->load = py::object();
        impl.reset();
        return fail("Python could not be started: " + message);
    }

    impl->released = std::make_unique<py::gil_scoped_release>();
    return std::unique_ptr<PythonHost>(new PythonHost(std::move(impl)));
}

PythonHost::PythonHost(std::unique_ptr<Impl> impl) : m_impl(std::move(impl)) {}
PythonHost::~PythonHost() = default;

int PythonHost::loadDirectory(const fs::path& dir, PluginManager& plugins, PluginSource source) {
    std::error_code ec;
    if (!fs::is_directory(dir, ec)) return 0;

    std::vector<fs::path> found;
    for (const fs::directory_entry& item : fs::directory_iterator(dir, ec))
        if (isPythonPlugin(item.path())) found.push_back(item.path());
    std::sort(found.begin(), found.end());

    int loaded = 0;
    for (const fs::path& path : found) {
        std::string error;
        if (loadFile(path, plugins, source, &error)) ++loaded;
        else report(path.filename().u8string(), error);
    }
    return loaded;
}

bool PythonHost::loadFile(const fs::path& path, PluginManager& plugins, PluginSource source, std::string* error) {
    ComPtr<IApplication> application;
    {
        py::gil_scoped_acquire gil;
        try {
            const std::string moduleName = moduleNameFor(path);
            py::object module = m_impl->load(fs::absolute(path).u8string(), moduleName);

            const std::string stem = path.stem().u8string();
            auto constant = [&](const char* name, const std::string& fallback) {
                const std::string value = py::hasattr(module, name) ? text(module.attr(name)) : std::string();
                return value.empty() ? fallback : value;
            };
            application = ComPtr<IApplication>::adopt(new PyApplication(
                module, constant("ID", "py." + stem), constant("NAME", stem), constant("VERSION", "0.0.0")));
        } catch (const std::exception& e) {
            if (error) *error = e.what();
            return false;
        }
    }
    // Outside the GIL scope: activation re-enters Python through the COM objects.
    return plugins.addApplication(std::move(application), error, source, path);
}

} // namespace plugin
} // namespace space

#endif // BUNGALOW_PYTHON
