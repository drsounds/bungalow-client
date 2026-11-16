#pragma once

#include "service/AccountManager.h"
#include "service/Service.h"

#include <shore/service_manager.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace space {
namespace service {

/// Shore's registry of the installed Services, which drives their lifecycle
/// and selects them by capability, plus the client's accounts:
///
///     for (MusicService* ms : services.getServices<MusicService>()) …
///     AuthAspect* auth = services.getService<AuthAspect>();
///
/// T may be Service itself, a concrete class or any capability interface,
/// including ones that don't derive from Service: a Service is offered for
/// every interface it implements. Every Service is an AuthAspect, so filter
/// on isLoggedIn()/login() rather than on the type.
class ServiceManager : public shore::SHServiceManager {
public:
    ServiceManager() = default;
    /// Stops and destroys every remaining Service.
    ~ServiceManager() override;

    /// Registers the Service (replacing one with the same id) and calls onCreate().
    Service* registerAspect(std::unique_ptr<Service> service) {
        Service* raw = service.get();
        return Register(std::move(service)) ? raw : nullptr;
    }
    /// registerAspect() that hands back the Service with its own type —
    /// capabilities derive from Service virtually, so the Service* that
    /// registerAspect() returns can't be static_cast back down.
    template <class T>
    T* registerService(std::unique_ptr<T> service) {
        T* typed = service.get();
        return registerAspect(std::move(service)) ? typed : nullptr;
    }

    /// Calls onStop() and onDestroy(), then destroys the Service.
    void unregisterAspect(const std::string& id) { Unregister(id); }

    Service* get(const std::string& id) const { return dynamic_cast<Service*>(Get(id)); }
    /// Every Service, in registration order.
    std::vector<Service*> all() const { return GetServices<Service>(); }

    /// onStart() on every Service, in registration order.
    void startAll() { StartAll(); }
    /// Whether startAll() ran (and stopAll() has not since): whoever registers
    /// a Service afterwards calls its onStart() itself.
    bool isStarted() const { return IsStarted(); }
    /// onStop() on every Service, in registration order.
    void stopAll() { StopAll(); }
    /// Stops, destroys and unregisters every Service.
    void shutdownAll() { ShutdownAll(); }

    /// Signs in to the Service with that id. Asynchronous: returns at once,
    /// then exactly one callback runs on the UI thread. On success the session
    /// is recorded with accounts() before onSuccess runs. An unknown id, a
    /// Service without sign-in or an invalid session report through onError.
    void login(const std::string& serviceId,
               std::function<void(const SPSession& session)> onSuccess,
               std::function<void(const std::string& error)> onError);
    /// Signs out of the Service and forgets its account.
    void logout(const std::string& serviceId);

    /// The sessions of the signed-in accounts (bungalow:accounts).
    AccountManager& accounts() { return *m_accounts; }
    const AccountManager& accounts() const { return *m_accounts; }

    /// The first Service exposing capability T, or nullptr.
    template <class T>
    T* getService() const { return GetService<T>(); }

    /// Every Service exposing capability T, in registration order.
    template <class T>
    std::vector<T*> getServices() const { return GetServices<T>(); }

private:
    // Shared, so a sign-in finishing after the manager is gone stays safe.
    std::shared_ptr<AccountManager> m_accounts = std::make_shared<AccountManager>();
};

} // namespace service
} // namespace space
