#pragma once

#include "aspect/BaseAspectManager.h"
#include "service/AccountManager.h"
#include "service/Service.h"

#include <functional>
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
/// including ones that don't derive from Service: a Service is offered for
/// every interface it implements. Every Service is an AuthAspect, so filter
/// on isLoggedIn()/login() rather than on the type.
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

private:
    // Shared, so a sign-in finishing after the manager is gone stays safe.
    std::shared_ptr<AccountManager> m_accounts = std::make_shared<AccountManager>();
};

} // namespace service
} // namespace space
