#pragma once

#include "aspect/BaseAspectManager.h"
#include "service/Service.h"

#include <memory>
#include <string>
#include <vector>

namespace space {
namespace service {

/// Registry of the installed Services, which drives their lifecycle and
/// selects them by capability:
///
///     for (MusicService* ms : services.getServices<MusicService>()) …
///     AuthAspect* auth = services.getService<AuthAspect>();
///
/// T may be Service itself, a concrete class or any capability interface,
/// including ones that don't derive from Service (AuthAspect): a Service is
/// offered for every interface it implements.
class ServiceManager : public aspect::BaseAspectManager<Service> {
public:
    ServiceManager() = default;
    /// Stops and destroys every remaining Service.
    ~ServiceManager() override;

    ServiceManager(const ServiceManager&) = delete;
    ServiceManager& operator=(const ServiceManager&) = delete;

    /// Registers the Service (replacing one with the same id) and calls onCreate().
    Service* registerAspect(std::unique_ptr<Service> service) override;
    /// registerAspect() that hands back the Service with its own type —
    /// capabilities derive from Service virtually, so the Service* that
    /// registerAspect() returns can't be static_cast back down.
    template <class T>
    T* registerService(std::unique_ptr<T> service) {
        T* typed = service.get();
        return registerAspect(std::move(service)) ? typed : nullptr;
    }

    /// Calls onStop() and onDestroy(), then destroys the Service.
    void unregisterAspect(const std::string& id) override;

    /// onStart() on every Service, in registration order.
    void startAll();
    /// onStop() on every Service, in registration order.
    void stopAll();
    /// Stops, destroys and unregisters every Service.
    void shutdownAll();

    /// The first Service exposing capability T, or nullptr.
    template <class T>
    T* getService() const {
        for (Service* s : all())
            if (T* t = dynamic_cast<T*>(s)) return t;
        return nullptr;
    }

    /// Every Service exposing capability T, in registration order.
    template <class T>
    std::vector<T*> getServices() const {
        std::vector<T*> result;
        for (Service* s : all())
            if (T* t = dynamic_cast<T*>(s)) result.push_back(t);
        return result;
    }
};

} // namespace service
} // namespace space
