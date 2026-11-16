#pragma once

#include "plugin/PluginSupport.h"
#include "service/BackgroundWorker.h"

#include <shore/plugin_manager.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace space {
namespace service { class ServiceManager; }

namespace plugin {

/// Where a plugin came from (legacy Java: ApplicationDescriptor.Source):
/// Builtin (handed to addApplication() by the host itself), AppDir (a library
/// shipped beside the application, <app>/Apps) or External (a library the
/// user installed, ~/Bungalow).
using PluginSource = shore::SHPluginSource;

/// What the manager knows about one plugin (legacy Java: ApplicationDescriptor).
using PluginInfo = shore::SHPluginInfo;

/// Shore's plugin host (shore::SHPluginManager; legacy Java:
/// ApplicationManager + ApplicationLoader) for the client: it discovers
/// plugins, loads their libraries and activates them. A plugin is a shared
/// library that exports BungalowCreateApplication (see
/// plugin/BungalowPlugin.h); the com::IApplication it returns registers
/// Services through a recording context, so deactivating the plugin removes
/// exactly what it added. Services are adapted to the client's capabilities
/// (playback, music lookup, sign-in) before they reach the ServiceManager.
///
/// Used on the UI thread. Must be destroyed (or unloadAll() called) before
/// the ServiceManager it registers into, and after anything that may still
/// be calling into a plugin's services on another thread.
class PluginManager : public shore::SHPluginManager {
public:
    /// `ui` posts to the UI thread; plugin services report through it.
    PluginManager(service::ServiceManager& services, service::Executor ui);
    ~PluginManager() override;

    /// Loads and activates every plugin library directly inside `dir`
    /// (sorted by name; other files are ignored). Returns how many loaded.
    int loadDirectory(const std::filesystem::path& dir, PluginSource source) { return LoadDirectory(dir, source); }

    /// Loads one library and activates its plugin. False, with a reason in
    /// `error`, when it is not a Bungalow plugin, was built for another ABI
    /// version or its id is already loaded.
    bool loadLibrary(const std::filesystem::path& file, PluginSource source, std::string* error = nullptr) {
        return LoadPlugin(file, source, error);
    }

    /// Adds a plugin that is already in the process and activates it. `source`
    /// and `path` say where it came from when it is not the host's own (a
    /// Python plugin, see PythonHost).
    bool addApplication(com::ComPtr<com::IApplication> application, std::string* error = nullptr,
                        PluginSource source = PluginSource::Builtin, const std::filesystem::path& path = {}) {
        return AddApplication(std::move(application), error, source, path);
    }

    /// Activates or deactivates a loaded plugin. Deactivating unregisters its
    /// services and forgets their accounts; the library stays loaded. Returns
    /// false for an unknown id or when activation fails.
    bool setEnabled(const std::string& id, bool enabled) { return SetEnabled(id, enabled); }

    std::vector<PluginInfo> plugins() const { return Plugins(); }

    /// Deactivates every plugin, newest first, and unloads the libraries.
    void unloadAll() { UnloadAll(); }

protected:
    com::Result OnRegisterService(PluginInfo& plugin, com::IService* service) override;
    void OnUnregisterService(PluginInfo& plugin, const std::string& serviceId) override;

private:
    service::ServiceManager& m_services;
    service::Executor        m_ui;
};

} // namespace plugin
} // namespace space
