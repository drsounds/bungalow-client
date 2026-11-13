#pragma once

#include "plugin/PluginManager.h"

#include <filesystem>
#include <memory>
#include <string>

namespace space {
namespace plugin {

/// The embedded Python interpreter, for plugins written in Python: the
/// lightweight alternative to a C++ plugin library. A Python plugin is one
/// .py file (or a package folder with an __init__.py) in a plugin folder:
///
///     import bungalow
///
///     ID, NAME, VERSION = "com.acme.stream", "Acme", "1.0.0"
///
///     class Acme(bungalow.Service):
///         id, name = "acme.stream", "Acme Stream"
///         def lookup(self, isrc): ...
///         def load_uri(self, uri): self.playback.state(bungalow.LOADING)
///         ...
///
///     def activate(context):
///         context.register_service(Acme())
///
/// Each one becomes a com::IApplication handed to the PluginManager, so its
/// services go through the same adapters as a C++ plugin's. See
/// docs/docs/architecture/plugins.md for the whole Python API.
///
/// Created and destroyed on the UI thread. Destroy it only after the
/// PluginManager it loaded into has unloaded (PluginManager::unloadAll()).
class PythonHost {
public:
    /// Whether this build embeds Python (BUNGALOW_ENABLE_PYTHON).
    static bool available();

    /// Starts the interpreter. `home` holds the standard library in a "Lib"
    /// folder (the build copies it to <app>/python). Null, with a reason in
    /// `error`, when this build has no Python or the interpreter fails to start.
    static std::unique_ptr<PythonHost> create(const std::filesystem::path& home, std::string* error = nullptr);

    ~PythonHost();
    PythonHost(const PythonHost&) = delete;
    PythonHost& operator=(const PythonHost&) = delete;

    /// Loads every Python plugin directly inside `dir`: *.py files and
    /// folders with an __init__.py, sorted by name; names starting with "_"
    /// or "." are skipped. Returns how many loaded.
    int loadDirectory(const std::filesystem::path& dir, PluginManager& plugins, PluginSource source);

    /// Loads one .py file or package folder and activates it.
    bool loadFile(const std::filesystem::path& path, PluginManager& plugins, PluginSource source,
                  std::string* error = nullptr);

private:
    struct Impl;
    explicit PythonHost(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> m_impl;
};

} // namespace plugin
} // namespace space
