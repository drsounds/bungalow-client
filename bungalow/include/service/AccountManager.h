#pragma once

#include "models/SPSession.h"

#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace space {
namespace service {

/// The sessions of the signed-in accounts, one per service id. Filled by
/// ServiceManager::login() as sign-ins succeed and shown at bungalow:accounts.
/// Sessions hold their data encrypted, so this is safe to keep and to persist
/// (see SPSession::encryptedData()).
class AccountManager {
public:
    /// Stores the session under its service id, replacing an earlier one.
    /// Invalid sessions are ignored.
    void put(const SPSession& session);
    /// Forgets the account of that service. Returns whether there was one.
    bool remove(const std::string& serviceId);

    std::optional<SPSession> get(const std::string& serviceId) const;
    bool isSignedIn(const std::string& serviceId) const;
    /// Every session, ordered by service id.
    std::vector<SPSession> all() const;

    /// Called after every put()/remove(), outside the lock.
    void setChangedCallback(std::function<void()> callback);

private:
    void notify();

    mutable std::mutex              m_mutex;
    std::map<std::string, SPSession> m_sessions;
    std::function<void()>           m_changed;
};

} // namespace service
} // namespace space
