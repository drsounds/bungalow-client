#pragma once

#include "models/SPSession.h"

#include <functional>
#include <string>

namespace space {
namespace service {

/// Authentication contract. Every Service derives from it, with empty
/// defaults, so a back-end that needs no sign-in (local playback) is simply
/// always signed out, and one that does overrides these. UI finds the
/// back-ends with an account via ServiceManager::getServices<AuthAspect>()
/// and checks isLoggedIn().
class AuthAspect {
public:
    virtual ~AuthAspect() = default;

    /// Presents whatever sign-in UI the back-end needs. Asynchronous: called
    /// on the UI thread, it returns at once and exactly one of the callbacks
    /// runs later, on the UI thread, when the attempt finishes. Prefer
    /// ServiceManager::login(), which also records the session with the
    /// AccountManager.
    virtual void login(std::function<void(const SPSession& session)> /*onSuccess*/,
                       std::function<void(const std::string& error)> /*onError*/) {}

    virtual void logout() {}

    /// True for back-ends that have accounts at all; the accounts view lists
    /// only these. Override together with login().
    virtual bool supportsLogin() const { return false; }

    virtual bool isLoggedIn() const { return false; }

    /// The current session, or an invalid (default) SPSession when signed out.
    virtual SPSession getSession() const { return {}; }

    /// Display name of the signed-in account, or empty when signed out.
    virtual std::string getAccountName() const { return getSession().user.name; }
};

} // namespace service
} // namespace space
