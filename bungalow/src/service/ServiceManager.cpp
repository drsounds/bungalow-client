#include "service/ServiceManager.h"

namespace space {
namespace service {

ServiceManager::~ServiceManager() {
    shutdownAll();
}

Service* ServiceManager::registerAspect(std::unique_ptr<Service> service) {
    Service* registered = BaseAspectManager<Service>::registerAspect(std::move(service));
    if (registered) registered->onCreate();
    return registered;
}

void ServiceManager::unregisterAspect(const std::string& id) {
    if (Service* s = get(id)) {
        s->onStop();
        s->onDestroy();
    }
    BaseAspectManager<Service>::unregisterAspect(id);
}

void ServiceManager::startAll() {
    for (Service* s : all()) s->onStart();
}

void ServiceManager::stopAll() {
    for (Service* s : all()) s->onStop();
}

void ServiceManager::shutdownAll() {
    // Newest first, so a Service can still reach the ones registered before it.
    std::vector<Service*> services = all();
    for (auto it = services.rbegin(); it != services.rend(); ++it)
        unregisterAspect((*it)->getId());
}

} // namespace service
} // namespace space
