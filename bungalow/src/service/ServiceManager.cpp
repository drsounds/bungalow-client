#include "service/ServiceManager.h"

namespace space {
namespace service {

// The registry and the lifecycle are Shore's (shore::SHServiceManager); what
// is left here is the client's sign-in.

ServiceManager::~ServiceManager() {
    // Here rather than in the base destructor, while the accounts still exist.
    ShutdownAll();
}

void ServiceManager::login(const std::string& serviceId,
                           std::function<void(const SPSession&)> onSuccess,
                           std::function<void(const std::string&)> onError) {
    Service* service = get(serviceId);
    if (!service) {
        if (onError) onError("Unknown service: " + serviceId);
        return;
    }
    std::shared_ptr<AccountManager> accounts = m_accounts;
    service->login(
        [accounts, serviceId, onSuccess, onError](const SPSession& session) {
            if (!session.isValid() || session.serviceId() != serviceId) {
                if (onError) onError("The service returned an invalid session");
                return;
            }
            accounts->put(session);
            if (onSuccess) onSuccess(session);
        },
        onError);
}

void ServiceManager::logout(const std::string& serviceId) {
    if (Service* service = get(serviceId)) service->logout();
    m_accounts->remove(serviceId);
}

} // namespace service
} // namespace space
