#pragma once

#include "models/SPSession.h"
#include "plugin/PluginSupport.h"
#include "service/BackgroundWorker.h"
#include "service/Service.h"

#include <memory>
#include <string>

namespace space {
namespace plugin {

/// Presents a plugin's com::IService as a native service::Service, so the
/// ServiceManager, PlaybackCoordinator and accounts view use it like a
/// built-in one. The capabilities follow the object's QueryInterface:
///
///     IMediaService + IMusicService  ->  service::MusicService
///     IMediaService                  ->  service::MediaService
///     IAuthAspect                    ->  the Service's sign-in functions
///
/// Sign-in results and playback events, which the plugin may raise on any
/// thread, are delivered through `ui`. Returns nullptr when the service has
/// no id.
std::unique_ptr<service::Service> wrapService(com::ComPtr<com::IService> service, service::Executor ui);

/// Copies a plugin's session into an SPSession of `serviceId`, encrypting its
/// data. Invalid when `session` is null or has no user id.
SPSession toSession(com::ISession* session, const std::string& serviceId);

} // namespace plugin
} // namespace space
