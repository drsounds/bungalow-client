#pragma once

#include "aspect/Aspect.h"
#include "service/AuthAspect.h"

#include <string>

namespace space {
namespace service {

/// Core contract for every back-end: a stable identity plus an Android-style
/// lifecycle. What a Service can *do* — play media, resolve music, browse a
/// catalogue — is expressed by also deriving from capability interfaces
/// (MediaService, MusicService, MusicCatalogueService). Sign-in comes from the
/// AuthAspect base, whose empty defaults mean "no account needed".
/// One object may combine any of them, and callers select back-ends by
/// capability through ServiceManager::getServices<T>(), never by vendor id.
///
/// Capability interfaces derive from Service *virtually*, so a class that
/// implements several of them still has a single Service base.
class Service : public aspect::Aspect, public AuthAspect {
public:
    /// Optional icon shown in service-selection UI; empty for none.
    virtual std::string getIconPath() const { return {}; }

    /// Called once, when the Service is registered with the ServiceManager.
    virtual void onCreate() {}
    /// Called by ServiceManager::startAll().
    virtual void onStart() {}
    /// Called by ServiceManager::stopAll(), and before onDestroy().
    virtual void onStop() {}
    /// Called once, just before the ServiceManager destroys the Service.
    virtual void onDestroy() {}
};

} // namespace service
} // namespace space
