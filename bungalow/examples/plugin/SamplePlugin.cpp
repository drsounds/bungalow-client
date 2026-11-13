// A complete Bungalow plugin: one streaming service with a sign-in, built as
// a shared library and loaded from <app>/Apps or ~/Bungalow. It needs only
// the two headers in bungalow/include/plugin. Also loaded by PluginTest.
//
// The service is a stand-in: it "plays" sample: URIs by reporting playback
// states, knows one recording and signs in without asking anything.

#include "plugin/BungalowPlugin.h"
#include "plugin/PluginSupport.h"

#include <cstring>
#include <mutex>
#include <string>

using namespace space::com;

namespace {

class SampleSession final : public ComObject<ISession> {
public:
    Result BUNGALOW_CALL GetUserId(IString** out) override { return returnString("u42", out); }
    Result BUNGALOW_CALL GetUserDisplayName(IString** out) override { return returnString("Sample User", out); }
    Result BUNGALOW_CALL GetData(IString** out) override { return returnString(R"({"token":"abc123"})", out); }
};

class SampleService final : public ComObject<IService, IAuthAspect, IMediaService, IMusicService> {
public:
    // IAspect
    Result BUNGALOW_CALL GetId(IString** out) override { return returnString("sample.stream", out); }
    Result BUNGALOW_CALL GetName(IString** out) override { return returnString("Sample Stream", out); }

    // IService
    Result BUNGALOW_CALL GetIconPath(IString** out) override {
        if (out) *out = nullptr;
        return kFalse;
    }
    Result BUNGALOW_CALL OnCreate() override { return kOk; }
    Result BUNGALOW_CALL OnStart() override { return kOk; }
    Result BUNGALOW_CALL OnStop() override { return kOk; }
    Result BUNGALOW_CALL OnDestroy() override { return kOk; }

    // IAuthAspect
    Result BUNGALOW_CALL SupportsLogin(int32_t* out) override {
        if (!out) return kPointer;
        *out = 1;
        return kOk;
    }
    Result BUNGALOW_CALL Login(ILoginCallback* callback) override {
        if (!callback) return kPointer;
        // A real service shows its sign-in page here, keeps `callback`
        // (AddRef) and reports from whatever thread the answer arrives on.
        m_signedIn = true;
        auto session = make<SampleSession>();
        return callback->OnSuccess(session.get());
    }
    Result BUNGALOW_CALL Logout() override {
        m_signedIn = false;
        return kOk;
    }
    Result BUNGALOW_CALL IsLoggedIn(int32_t* out) override {
        if (!out) return kPointer;
        *out = m_signedIn ? 1 : 0;
        return kOk;
    }
    Result BUNGALOW_CALL GetSession(ISession** out) override {
        if (!out) return kPointer;
        *out = m_signedIn ? new SampleSession() : nullptr;
        return m_signedIn ? kOk : kFalse;
    }

    // IMediaService
    Result BUNGALOW_CALL SetPlaybackListener(IPlaybackListener* listener) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_listener = ComPtr<IPlaybackListener>(listener);
        return kOk;
    }
    Result BUNGALOW_CALL AcceptsUri(const char* uri, int32_t* out) override {
        if (!uri || !out) return kPointer;
        *out = std::strncmp(uri, "sample:", 7) == 0 ? 1 : 0;
        return kOk;
    }
    Result BUNGALOW_CALL LoadUri(const char* uri) override {
        if (!uri) return kPointer;
        ComPtr<IPlaybackListener> listener = this->listener();
        if (!listener) return kOk;
        if (std::strcmp(uri, "sample:track:1") != 0) return listener->OnError("No such sample track");
        listener->OnStateChanged(kPlaybackLoading);
        listener->OnTrackChanged("Sample Song", "Sample Artist", "Sample Album");
        return kOk;
    }
    Result BUNGALOW_CALL Play() override { return state(kPlaybackPlaying); }
    Result BUNGALOW_CALL Pause() override { return state(kPlaybackPaused); }
    Result BUNGALOW_CALL Stop() override { return state(kPlaybackStopped); }
    Result BUNGALOW_CALL Seek(int32_t positionMs) override {
        m_positionMs = positionMs;
        return kOk;
    }
    Result BUNGALOW_CALL GetPositionMs(int32_t* out) override {
        if (!out) return kPointer;
        *out = m_positionMs;
        return kOk;
    }
    Result BUNGALOW_CALL GetDurationMs(int32_t* out) override {
        if (!out) return kPointer;
        *out = 180000;
        return kOk;
    }

    // IMusicService: called on a background thread.
    Result BUNGALOW_CALL Lookup(const char* isrc, IString** outUri, IString** outTitle,
                                IString** outArtist) override {
        if (!isrc || std::strcmp(isrc, "SAMPLE0000001") != 0) return kFalse;
        return found(outUri, outTitle, outArtist);
    }
    Result BUNGALOW_CALL LookupByTitleArtist(const char* title, const char* /*artist*/, IString** outUri,
                                             IString** outTitle, IString** outArtist) override {
        if (!title || std::strcmp(title, "Sample Song") != 0) return kFalse;
        return found(outUri, outTitle, outArtist);
    }

private:
    ComPtr<IPlaybackListener> listener() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_listener;
    }
    Result state(int32_t value) {
        if (ComPtr<IPlaybackListener> l = listener()) l->OnStateChanged(value);
        return kOk;
    }
    static Result found(IString** outUri, IString** outTitle, IString** outArtist) {
        returnString("sample:track:1", outUri);
        returnString("Sample Song", outTitle);
        returnString("Sample Artist", outArtist);
        return kOk;
    }

    std::mutex                m_mutex;
    ComPtr<IPlaybackListener> m_listener;
    bool                      m_signedIn = false;
    int32_t                   m_positionMs = 0;
};

class SampleApplication final : public ComObject<IApplication> {
public:
    Result BUNGALOW_CALL GetId(IString** out) override { return returnString("se.bungalow.sample", out); }
    Result BUNGALOW_CALL GetName(IString** out) override { return returnString("Sample Plugin", out); }
    Result BUNGALOW_CALL GetVersion(IString** out) override { return returnString("1.0.0", out); }

    Result BUNGALOW_CALL OnActivate(IApplicationContext* context) override {
        if (!context) return kPointer;
        auto service = make<SampleService>();
        // The host keeps its own reference; ours goes when `service` does.
        return context->RegisterService(service.get());
    }
    Result BUNGALOW_CALL OnDeactivate() override { return kOk; }
};

} // namespace

BUNGALOW_PLUGIN_EXPORT Result BungalowCreateApplication(uint32_t abiVersion, IApplication** out) {
    if (!out) return kPointer;
    *out = nullptr;
    if (abiVersion != kAbiVersion) return kFail;
    *out = new SampleApplication();
    return kOk;
}
