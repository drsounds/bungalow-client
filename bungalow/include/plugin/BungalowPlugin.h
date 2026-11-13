#pragma once

// The Bungalow plugin ABI: COM interfaces through which an externally loaded
// library contributes Services (and, later, Features and Concepts) to the
// client. They are the COM counterparts of the native classes:
//
//     aspect::Aspect      <->  IAspect
//     service::Service    <->  IService         (+ IMediaService, IMusicService)
//     service::AuthAspect <->  IAuthAspect
//     feature::Feature    <->  IFeature
//     concepts::Concept   <->  IConcept
//     the plugin itself   <->  IApplication     (legacy Java: app.Application)
//
// This header is self-contained (no STL, no other Bungalow header), so a
// plugin needs nothing else; plugin/PluginSupport.h adds optional helpers.
//
// COM rules that apply:
//   * IUnknown here is binary compatible with the Windows IUnknown: same
//     vtable order, same IID, stdcall, so an object may implement both.
//   * Every method returns a Result (an HRESULT). Never throw across it.
//   * Out parameters that are interfaces are returned AddRef'ed; the caller
//     Releases them. In parameters are only borrowed: AddRef to keep one.
//   * Capabilities are found with QueryInterface: an IService that plays
//     music also answers IID_IMediaService and IID_IMusicService, one with
//     accounts answers IID_IAuthAspect.
//   * Strings are UTF-8. `const char*` in parameters are valid for the call
//     only; strings going out are IString objects.
//
// Threading: the host calls IApplication, lifecycle, playback and sign-in
// methods on its UI thread. IMusicService lookups are called on a background
// thread and must be thread-safe. A plugin may invoke ILoginCallback and
// IPlaybackListener from any thread; the host moves them to its UI thread.

#include <stdint.h>

#if defined(_WIN32)
#define BUNGALOW_CALL __stdcall
#define BUNGALOW_PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
#define BUNGALOW_CALL
#define BUNGALOW_PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))
#endif

namespace space {
namespace com {

/// Bumped on every incompatible change of this header. The host passes its
/// own to BungalowCreateApplication; a plugin built for another one fails.
constexpr uint32_t kAbiVersion = 1;

/// An HRESULT: negative values are failures.
using Result = int32_t;
constexpr Result kOk          = 0;                               // S_OK
constexpr Result kFalse       = 1;                               // S_FALSE: fine, but nothing to return
constexpr Result kNotImpl     = static_cast<Result>(0x80004001); // E_NOTIMPL
constexpr Result kNoInterface = static_cast<Result>(0x80004002); // E_NOINTERFACE
constexpr Result kPointer     = static_cast<Result>(0x80004003); // E_POINTER
constexpr Result kFail        = static_cast<Result>(0x80004005); // E_FAIL
constexpr Result kInvalidArg  = static_cast<Result>(0x80070057); // E_INVALIDARG
constexpr bool succeeded(Result r) { return r >= 0; }
constexpr bool failed(Result r) { return r < 0; }

/// Same layout as the Windows GUID.
struct Guid {
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t  data4[8];
};
constexpr bool operator==(const Guid& a, const Guid& b) {
    return a.data1 == b.data1 && a.data2 == b.data2 && a.data3 == b.data3 &&
           a.data4[0] == b.data4[0] && a.data4[1] == b.data4[1] && a.data4[2] == b.data4[2] &&
           a.data4[3] == b.data4[3] && a.data4[4] == b.data4[4] && a.data4[5] == b.data4[5] &&
           a.data4[6] == b.data4[6] && a.data4[7] == b.data4[7];
}
constexpr bool operator!=(const Guid& a, const Guid& b) { return !(a == b); }

inline constexpr Guid IID_IUnknown{0x00000000, 0x0000, 0x0000, {0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
inline constexpr Guid IID_IString{0x56c238d5, 0x9ec7, 0x4ea2, {0x9e, 0xbd, 0x27, 0x53, 0xf1, 0x55, 0x1a, 0xc9}};
inline constexpr Guid IID_IAspect{0xe829ebef, 0x668d, 0x4d1f, {0xa7, 0x73, 0xdd, 0xb8, 0xf8, 0x0f, 0xfa, 0xdb}};
inline constexpr Guid IID_IService{0xf99f1dde, 0xdcdc, 0x46c0, {0xa6, 0x8b, 0xbe, 0xd0, 0x18, 0xf5, 0xbe, 0xb7}};
inline constexpr Guid IID_IAuthAspect{0x4702d575, 0x4e7e, 0x4611, {0x91, 0x83, 0x27, 0x2c, 0xab, 0xc3, 0x18, 0xbd}};
inline constexpr Guid IID_ISession{0xd780cd0f, 0xf956, 0x4231, {0xa7, 0x51, 0x3c, 0xa3, 0x03, 0x87, 0xa1, 0xa4}};
inline constexpr Guid IID_ILoginCallback{0xf221ccde, 0xb9ab, 0x46f0, {0xb9, 0xca, 0xe7, 0x6c, 0x52, 0x18, 0x4e, 0x92}};
inline constexpr Guid IID_IMediaService{0x59f156fd, 0x78f4, 0x4316, {0xb0, 0xf3, 0x61, 0x65, 0x67, 0x18, 0xe9, 0x14}};
inline constexpr Guid IID_IPlaybackListener{0x9d4d8922, 0x5057, 0x4df2, {0x9a, 0x0e, 0xf7, 0x32, 0x45, 0x69, 0x2b, 0x89}};
inline constexpr Guid IID_IMusicService{0x946fa9ce, 0x7ce0, 0x408b, {0x90, 0xbd, 0x2a, 0x95, 0x3d, 0x19, 0x00, 0x82}};
inline constexpr Guid IID_IFeature{0x88ec763d, 0xe71c, 0x47be, {0x95, 0xc6, 0xda, 0xb7, 0x85, 0x16, 0x85, 0x71}};
inline constexpr Guid IID_IConcept{0x0373e8b4, 0x8582, 0x4490, {0xbc, 0xab, 0x5c, 0x6b, 0x9c, 0xaf, 0x3c, 0x17}};
inline constexpr Guid IID_IConceptContext{0xc9beb373, 0x6f92, 0x4401, {0xbb, 0x90, 0x14, 0xfb, 0x5f, 0x4c, 0x61, 0x67}};
inline constexpr Guid IID_IApplication{0xc3f0a4ff, 0x8969, 0x4e41, {0x87, 0x26, 0x6b, 0x49, 0x9a, 0xda, 0xbe, 0xea}};
inline constexpr Guid IID_IApplicationContext{0xb8797ef9, 0x8cd8, 0x4d9d, {0x81, 0x42, 0xb2, 0x53, 0xc7, 0xf4, 0xc0, 0x66}};

// ── Foundation ───────────────────────────────────────────────────────────────

struct IUnknown {
    virtual Result   BUNGALOW_CALL QueryInterface(const Guid& iid, void** out) = 0;
    virtual uint32_t BUNGALOW_CALL AddRef() = 0;
    virtual uint32_t BUNGALOW_CALL Release() = 0;
};

/// An immutable UTF-8 string, owned by whoever made it.
struct IString : IUnknown {
    /// Null-terminated; valid until the string is released.
    virtual const char* BUNGALOW_CALL Data() = 0;
    virtual uint32_t    BUNGALOW_CALL Size() = 0;
};

// ── aspect::Aspect ───────────────────────────────────────────────────────────

struct IAspect : IUnknown {
    /// Stable id, unique among aspects of the same kind (e.g. "acme.stream").
    virtual Result BUNGALOW_CALL GetId(IString** out) = 0;
    virtual Result BUNGALOW_CALL GetName(IString** out) = 0;
};

// ── service::AuthAspect ──────────────────────────────────────────────────────

/// A sign-in: who is signed in, plus what the service needs to resume it.
struct ISession : IUnknown {
    virtual Result BUNGALOW_CALL GetUserId(IString** out) = 0;
    virtual Result BUNGALOW_CALL GetUserDisplayName(IString** out) = 0;
    /// The session data (tokens, …) as a JSON document. The host encrypts it
    /// as soon as it receives it. kFalse when there is none.
    virtual Result BUNGALOW_CALL GetData(IString** outJson) = 0;
};

/// Result of IAuthAspect::Login. Call exactly one method, once, from any thread.
struct ILoginCallback : IUnknown {
    virtual Result BUNGALOW_CALL OnSuccess(ISession* session) = 0;
    virtual Result BUNGALOW_CALL OnError(const char* message) = 0;
};

struct IAuthAspect : IUnknown {
    /// *out != 0 when the back-end has accounts at all.
    virtual Result BUNGALOW_CALL SupportsLogin(int32_t* out) = 0;
    /// Starts a sign-in and returns at once; the outcome goes to `callback`,
    /// which the plugin AddRefs for as long as the attempt runs. A failure
    /// returned here means the attempt never started (no callback follows).
    virtual Result BUNGALOW_CALL Login(ILoginCallback* callback) = 0;
    virtual Result BUNGALOW_CALL Logout() = 0;
    virtual Result BUNGALOW_CALL IsLoggedIn(int32_t* out) = 0;
    /// kFalse (and *out == nullptr) when signed out.
    virtual Result BUNGALOW_CALL GetSession(ISession** out) = 0;
};

// ── service::Service and its capabilities ────────────────────────────────────

struct IService : IAspect {
    /// kFalse for no icon.
    virtual Result BUNGALOW_CALL GetIconPath(IString** out) = 0;
    virtual Result BUNGALOW_CALL OnCreate() = 0;
    virtual Result BUNGALOW_CALL OnStart() = 0;
    virtual Result BUNGALOW_CALL OnStop() = 0;
    virtual Result BUNGALOW_CALL OnDestroy() = 0;
};

/// Values of service::MediaService::PlaybackState.
enum PlaybackState : int32_t {
    kPlaybackIdle = 0,
    kPlaybackLoading,
    kPlaybackPlaying,
    kPlaybackPaused,
    kPlaybackStopped,
    kPlaybackError,
};

/// Implemented by the host; a media service reports its playback here.
struct IPlaybackListener : IUnknown {
    virtual Result BUNGALOW_CALL OnStateChanged(int32_t state) = 0;
    virtual Result BUNGALOW_CALL OnPositionChanged(int32_t positionMs, int32_t durationMs) = 0;
    virtual Result BUNGALOW_CALL OnTrackChanged(const char* title, const char* artist, const char* album) = 0;
    /// Also moves the service to kPlaybackError.
    virtual Result BUNGALOW_CALL OnError(const char* message) = 0;
    /// The loaded track reached its natural end.
    virtual Result BUNGALOW_CALL OnCompleted() = 0;
};

/// service::MediaService: transport control over one loaded URI.
struct IMediaService : IUnknown {
    /// The host's listener, or nullptr to detach. One at a time.
    virtual Result BUNGALOW_CALL SetPlaybackListener(IPlaybackListener* listener) = 0;
    virtual Result BUNGALOW_CALL AcceptsUri(const char* uri, int32_t* out) = 0;
    /// Prepares `uri` without starting it; failures go to OnError.
    virtual Result BUNGALOW_CALL LoadUri(const char* uri) = 0;
    virtual Result BUNGALOW_CALL Play() = 0;
    virtual Result BUNGALOW_CALL Pause() = 0;
    virtual Result BUNGALOW_CALL Stop() = 0;
    virtual Result BUNGALOW_CALL Seek(int32_t positionMs) = 0;
    virtual Result BUNGALOW_CALL GetPositionMs(int32_t* out) = 0;
    virtual Result BUNGALOW_CALL GetDurationMs(int32_t* out) = 0;
};

/// service::MusicService: can this back-end play a given recording? Both
/// lookups return kFalse when it cannot; otherwise a URI its own
/// IMediaService::LoadUri plays, plus the title and artist it matched.
/// Called on a background thread.
struct IMusicService : IUnknown {
    virtual Result BUNGALOW_CALL Lookup(const char* isrc, IString** outUri, IString** outTitle,
                                        IString** outArtist) = 0;
    /// `artist` may be empty.
    virtual Result BUNGALOW_CALL LookupByTitleArtist(const char* title, const char* artist,
                                                     IString** outUri, IString** outTitle,
                                                     IString** outArtist) = 0;
};

// ── feature::Feature and concepts::Concept ───────────────────────────────────
// Defined so the contract is complete; the host does not accept them yet
// (IApplicationContext returns kNotImpl), because native views cannot cross
// the plugin boundary.

struct IFeature : IAspect {
    virtual Result BUNGALOW_CALL GetSidebarNodeCount(uint32_t* out) = 0;
    /// One sidebar entry: the section it goes in ("LIBRARY"), its title, the
    /// bungalow: URI it navigates to and an icon path (may be empty).
    virtual Result BUNGALOW_CALL GetSidebarNode(uint32_t index, IString** outSection, IString** outTitle,
                                                IString** outUri, IString** outIconPath) = 0;
};

struct IConceptContext : IUnknown {
    virtual Result BUNGALOW_CALL GetConceptId(IString** out) = 0;
    virtual Result BUNGALOW_CALL RegisterFeature(IFeature* feature) = 0;
};

struct IConcept : IAspect {
    virtual Result BUNGALOW_CALL GetIconPath(IString** out) = 0;
    virtual Result BUNGALOW_CALL OnActivate(IConceptContext* context) = 0;
    virtual Result BUNGALOW_CALL OnDeactivate() = 0;
};

// ── The plugin ───────────────────────────────────────────────────────────────

/// Registration surface handed to IApplication::OnActivate. The host records
/// every contribution and removes it again when the plugin is deactivated.
/// Valid during OnActivate only.
struct IApplicationContext : IUnknown {
    /// Id of the plugin being activated.
    virtual Result BUNGALOW_CALL GetApplicationId(IString** out) = 0;
    /// Adds the service to the host's ServiceManager (OnCreate, then OnStart
    /// once services run). Fails when the id is already taken.
    virtual Result BUNGALOW_CALL RegisterService(IService* service) = 0;
    virtual Result BUNGALOW_CALL RegisterFeature(IFeature* feature) = 0;
    virtual Result BUNGALOW_CALL RegisterConcept(IConcept* concept_) = 0;
};

/// A plugin's entry object. GetId() is its identity, a reverse-DNS name such
/// as "com.acme.stream".
struct IApplication : IAspect {
    virtual Result BUNGALOW_CALL GetVersion(IString** out) = 0;
    /// Register the plugin's services through `context`.
    virtual Result BUNGALOW_CALL OnActivate(IApplicationContext* context) = 0;
    /// Release what OnActivate acquired. The host has already removed and
    /// released everything registered through the context.
    virtual Result BUNGALOW_CALL OnDeactivate() = 0;
};

/// Signature of the one function a plugin library exports, under the name
/// "BungalowCreateApplication":
///
///     BUNGALOW_PLUGIN_EXPORT space::com::Result
///     BungalowCreateApplication(uint32_t abiVersion, space::com::IApplication** out);
///
/// It returns kFail when abiVersion != kAbiVersion, else a new IApplication.
/// (Plain C calling convention, so the exported name is never decorated.)
using CreateApplicationFn = Result (*)(uint32_t abiVersion, IApplication** out);
constexpr const char* kCreateApplicationSymbol = "BungalowCreateApplication";

} // namespace com
} // namespace space
