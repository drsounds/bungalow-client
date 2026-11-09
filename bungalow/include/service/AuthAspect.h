#pragma once

#include <functional>
#include <string>

namespace space {
namespace service {

/// Authentication capability. Implemented, next to Service, by back-ends that
/// need a signed-in account (streaming services, stores). Back-ends that need
/// no sign-in, such as local playback, simply don't implement it, so UI finds
/// the ones that do with ServiceManager::getServices<AuthAspect>().
class AuthAspect {
public:
    virtual ~AuthAspect() = default;

    virtual bool isAuthenticated() const = 0;

    /// Presents whatever sign-in UI the back-end needs. Called on the UI
    /// thread; exactly one of the callbacks runs when the attempt finishes.
    virtual void login(std::function<void()> onSuccess,
                       std::function<void(const std::string& error)> onError) = 0;

    virtual void logout() = 0;

    /// Display name of the signed-in account, or empty when signed out.
    virtual std::string getAccountName() const = 0;
};

} // namespace service
} // namespace space
