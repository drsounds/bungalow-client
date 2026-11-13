#pragma once

#include "plugin/PluginSupport.h"
#include "service/BackgroundWorker.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace space {
namespace service { class ServiceManager; }

namespace plugin {

/// Where a plugin came from (legacy Java: ApplicationDescriptor.Source).
enum class PluginSource {
    Builtin,  ///< Handed to addApplication() by the host itself.
    AppDir,   ///< A library shipped beside the application (<app>/Apps).
    External, ///< A library the user installed (~/Bungalow).
};

/// What the manager knows about one plugin (legacy Java: ApplicationDescriptor).
struct PluginInfo {
    std::string              id;
    std::string              name;
    std::string              version;
    PluginSource             source = PluginSource::Builtin;
    std::filesystem::path    path;       ///< The library or Python file; empty for Builtin.
    bool                     active = false;
    std::vector<std::string> serviceIds; ///< Services it registered while active.
};

/// Discovers plugins, loads their libraries and activates them (legacy Java:
/// ApplicationManager + ApplicationLoader). A plugin is a shared library that
/// exports BungalowCreateApplication (see plugin/BungalowPlugin.h); the
/// com::IApplication it returns registers Services through a recording
/// context, so deactivating the plugin removes exactly what it added.
///
/// Used on the UI thread. Must be destroyed (or unloadAll() called) before
/// the ServiceManager it registers into, and after anything that may still
/// be calling into a plugin's services on another thread.
class PluginManager {
public:
    /// `ui` posts to the UI thread; plugin services report through it.
    PluginManager(service::ServiceManager& services, service::Executor ui);
    ~PluginManager();

    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    /// Loads and activates every plugin library directly inside `dir`
    /// (sorted by name; other files are ignored). Returns how many loaded.
    int loadDirectory(const std::filesystem::path& dir, PluginSource source);

    /// Loads one library and activates its plugin. False, with a reason in
    /// `error`, when it is not a Bungalow plugin, was built for another ABI
    /// version or its id is already loaded.
    bool loadLibrary(const std::filesystem::path& file, PluginSource source, std::string* error = nullptr);

    /// Adds a plugin that is already in the process and activates it. `source`
    /// and `path` say where it came from when it is not the host's own (a
    /// Python plugin, see PythonHost).
    bool addApplication(com::ComPtr<com::IApplication> application, std::string* error = nullptr,
                        PluginSource source = PluginSource::Builtin, const std::filesystem::path& path = {});

    /// Activates or deactivates a loaded plugin. Deactivating unregisters its
    /// services and forgets their accounts; the library stays loaded. Returns
    /// false for an unknown id or when activation fails.
    bool setEnabled(const std::string& id, bool enabled);

    std::vector<PluginInfo> plugins() const;

    /// Deactivates every plugin, newest first, and unloads the libraries.
    void unloadAll();

private:
    struct Entry;
    class Context;

    bool add(com::ComPtr<com::IApplication> application, PluginSource source,
             const std::filesystem::path& path, void* library, std::string* error);
    Entry* find(const std::string& id) const;
    bool activate(Entry& entry);
    void deactivate(Entry& entry);
    com::Result registerService(Entry& entry, com::IService* service);

    service::ServiceManager&            m_services;
    service::Executor                   m_ui;
    std::vector<std::unique_ptr<Entry>> m_entries;
};

} // namespace plugin
} // namespace space
