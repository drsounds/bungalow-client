#include "plugin/PluginManager.h"

#include "plugin/ComServiceAdapter.h"
#include "service/ServiceManager.h"

namespace space {
namespace plugin {

using namespace com;

// Discovery, library loading, activation and the recording context are
// Shore's (shore::SHPluginManager). The client supplies its entry symbol and
// how a plugin's services become native ones.

namespace {

shore::SHPluginManager::Options hostOptions() {
    shore::SHPluginManager::Options options;
    options.entrySymbol = kCreateApplicationSymbol;
    options.abiVersion = kAbiVersion;
    options.hostName = "Bungalow";
    return options;
}

} // namespace

PluginManager::PluginManager(service::ServiceManager& services, service::Executor ui)
    : shore::SHPluginManager(&services, hostOptions()), m_services(services), m_ui(std::move(ui)) {}

PluginManager::~PluginManager() {
    // Here rather than in the base destructor: unloading calls the hooks below.
    UnloadAll();
}

Result PluginManager::OnRegisterService(PluginInfo& plugin, IService* service) {
    if (!service) return kPointer;
    std::unique_ptr<service::Service> adapter = wrapService(ComPtr<IService>(service), m_ui);
    if (!adapter) return kInvalidArg;

    // A plugin may not replace a service that is already there.
    const std::string id = adapter->getId();
    if (m_services.get(id)) return kFail;

    service::Service* registered = m_services.registerAspect(std::move(adapter));
    if (!registered) return kFail;
    plugin.serviceIds.push_back(id);
    if (m_services.isStarted()) registered->onStart();
    return kOk;
}

void PluginManager::OnUnregisterService(PluginInfo&, const std::string& serviceId) {
    m_services.accounts().remove(serviceId);
    m_services.unregisterAspect(serviceId); // onStop, onDestroy, then releases the plugin's object
}

} // namespace plugin
} // namespace space
