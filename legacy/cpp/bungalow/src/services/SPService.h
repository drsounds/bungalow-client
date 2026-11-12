#pragma once

#include <string>

/// Base class for all Bungalow services (media, streaming, social, etc.).
class SPService {
public:
    virtual ~SPService() = default;

    /// Returns true if this service can handle the given URI scheme/host.
    virtual bool acceptsUri(const std::string& uri) const = 0;

    /// Initiates the login flow (opens browser, shows dialog, etc.).
    virtual void startLogin() = 0;

    /// Logs the current user out and clears any stored credentials.
    virtual void logout() = 0;

    /// Stable machine-readable identifier (e.g. "local", "spotify").
    virtual std::string getId() const = 0;

    /// Human-readable display name (e.g. "Local Media", "Spotify").
    virtual std::string getName() const = 0;

    /// Returns true when the service has an active authenticated session.
    virtual bool isLoggedIn() const = 0;

    /// Returns the display name / username of the currently logged-in user,
    /// or an empty string when not logged in.
    virtual std::string getLoggedInUser() const = 0;
};
