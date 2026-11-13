#include "plugin/ComServiceAdapter.h"

#include "service/MediaService.h"
#include "service/MusicService.h"

#include <nlohmann/json.hpp>

#include <atomic>
#include <functional>
#include <optional>

namespace space {
namespace plugin {

using namespace com;
using service::Executor;

SPSession toSession(ISession* session, const std::string& serviceId) {
    if (!session) return {};
    SPUser user;
    user.id = getString([&](IString** o) { return session->GetUserId(o); });
    user.name = getString([&](IString** o) { return session->GetUserDisplayName(o); });
    if (!user.isValid()) return {};

    SPSession result(std::move(user), serviceId);
    ComPtr<IString> data;
    if (session->GetData(data.put()) == kOk && data) {
        const std::string text = toStd(data.get());
        nlohmann::json parsed = nlohmann::json::parse(text, nullptr, false);
        // Not JSON after all: keep it, as a JSON string.
        result.setData(parsed.is_discarded() ? nlohmann::json(text) : parsed);
    }
    return result;
}

namespace {

std::string orEmpty(const char* s) { return s ? s : ""; }

// ── Sign-in callback handed to the plugin ────────────────────────────────────

class LoginCallback final : public ComObject<ILoginCallback> {
public:
    LoginCallback(Executor ui, std::string serviceId, std::function<void(const SPSession&)> onSuccess,
                  std::function<void(const std::string&)> onError)
        : m_ui(std::move(ui)), m_serviceId(std::move(serviceId)), m_onSuccess(std::move(onSuccess)),
          m_onError(std::move(onError)) {}

    Result BUNGALOW_CALL OnSuccess(ISession* session) override {
        // Copied (and encrypted) here: `session` is only valid for this call.
        SPSession converted = toSession(session, m_serviceId);
        if (!converted.isValid()) return fail("The service returned an invalid session") ? kInvalidArg : kFail;
        if (m_done.exchange(true)) return kFail;
        m_ui([onSuccess = std::move(m_onSuccess), converted] {
            if (onSuccess) onSuccess(converted);
        });
        return kOk;
    }

    Result BUNGALOW_CALL OnError(const char* message) override {
        return fail(orEmpty(message)) ? kOk : kFail;
    }

    /// Reports a failure unless an outcome was already reported.
    bool fail(std::string message) {
        if (m_done.exchange(true)) return false;
        m_ui([onError = std::move(m_onError), message = std::move(message)] {
            if (onError) onError(message);
        });
        return true;
    }

private:
    Executor                               m_ui;
    std::string                            m_serviceId;
    std::function<void(const SPSession&)>  m_onSuccess;
    std::function<void(const std::string&)> m_onError;
    std::atomic<bool>                      m_done{false};
};

// ── Playback listener handed to the plugin ───────────────────────────────────

/// Where a HostPlaybackListener delivers, on the UI thread.
struct PlaybackSink {
    virtual void deliverState(int32_t state) = 0;
    virtual void deliverPosition(int32_t positionMs, int32_t durationMs) = 0;
    virtual void deliverTrack(const std::string& title, const std::string& artist, const std::string& album) = 0;
    virtual void deliverError(const std::string& message) = 0;
    virtual void deliverCompleted() = 0;

protected:
    ~PlaybackSink() = default;
};

class HostPlaybackListener final : public ComObject<IPlaybackListener> {
public:
    HostPlaybackListener(Executor ui, PlaybackSink* sink) : m_ui(std::move(ui)), m_sink(sink) {}

    /// UI thread. Events still queued, or raised later, are dropped.
    void detach() { m_sink = nullptr; }

    Result BUNGALOW_CALL OnStateChanged(int32_t state) override {
        return post([state](PlaybackSink& s) { s.deliverState(state); });
    }
    Result BUNGALOW_CALL OnPositionChanged(int32_t positionMs, int32_t durationMs) override {
        return post([positionMs, durationMs](PlaybackSink& s) { s.deliverPosition(positionMs, durationMs); });
    }
    Result BUNGALOW_CALL OnTrackChanged(const char* title, const char* artist, const char* album) override {
        return post([t = orEmpty(title), a = orEmpty(artist), al = orEmpty(album)](PlaybackSink& s) {
            s.deliverTrack(t, a, al);
        });
    }
    Result BUNGALOW_CALL OnError(const char* message) override {
        return post([m = orEmpty(message)](PlaybackSink& s) { s.deliverError(m); });
    }
    Result BUNGALOW_CALL OnCompleted() override {
        return post([](PlaybackSink& s) { s.deliverCompleted(); });
    }

private:
    template <class Fn> Result post(Fn fn) {
        // The task keeps the listener alive; m_sink is only read on the UI thread.
        m_ui([self = ComPtr<HostPlaybackListener>(this), fn = std::move(fn)] {
            if (self->m_sink) fn(*self->m_sink);
        });
        return kOk;
    }

    Executor      m_ui;
    PlaybackSink* m_sink;
};

// ── The adapters ─────────────────────────────────────────────────────────────

struct Core {
    ComPtr<IService>      service;
    ComPtr<IAuthAspect>   auth;
    ComPtr<IMediaService> media;
    ComPtr<IMusicService> music;
    Executor              ui;
    std::string           id;
    std::string           name;
};

/// Identity, lifecycle and sign-in. Base is Service, MediaService or MusicService.
template <class Base> class ComServiceImpl : public Base {
public:
    explicit ComServiceImpl(Core core) : m(std::move(core)) {}

    std::string getId() const override { return m.id; }
    std::string getName() const override { return m.name; }
    std::string getIconPath() const override {
        return getString([&](IString** o) { return m.service->GetIconPath(o); });
    }

    void onCreate() override { m.service->OnCreate(); }
    void onStart() override { m.service->OnStart(); }
    void onStop() override { m.service->OnStop(); }
    void onDestroy() override { m.service->OnDestroy(); }

    bool supportsLogin() const override {
        int32_t value = 0;
        return m.auth && succeeded(m.auth->SupportsLogin(&value)) && value != 0;
    }

    void login(std::function<void(const SPSession&)> onSuccess,
               std::function<void(const std::string&)> onError) override {
        auto callback = make<LoginCallback>(m.ui, m.id, std::move(onSuccess), std::move(onError));
        if (!m.auth) {
            callback->fail("This service has no sign-in");
        } else if (failed(m.auth->Login(callback.get()))) {
            callback->fail("The sign-in could not be started");
        }
    }

    void logout() override {
        if (m.auth) m.auth->Logout();
    }

    bool isLoggedIn() const override {
        int32_t value = 0;
        return m.auth && succeeded(m.auth->IsLoggedIn(&value)) && value != 0;
    }

    SPSession getSession() const override {
        ComPtr<ISession> session;
        if (!m.auth || m.auth->GetSession(session.put()) != kOk) return {};
        return toSession(session.get(), m.id);
    }

protected:
    Core m;
};

using PlaybackState = service::MediaService::PlaybackState;

/// Adds transport control and playback events. Base is MediaService or MusicService.
template <class Base> class ComMediaImpl : public ComServiceImpl<Base>, private PlaybackSink {
public:
    explicit ComMediaImpl(Core core) : ComServiceImpl<Base>(std::move(core)) {
        m_listener = make<HostPlaybackListener>(this->m.ui, static_cast<PlaybackSink*>(this));
        this->m.media->SetPlaybackListener(m_listener.get());
    }
    ~ComMediaImpl() override { detachListener(); }

    void onDestroy() override {
        detachListener(); // before the service tears itself down
        ComServiceImpl<Base>::onDestroy();
    }

    bool acceptsUri(const std::string& uri) const override {
        int32_t value = 0;
        return succeeded(this->m.media->AcceptsUri(uri.c_str(), &value)) && value != 0;
    }
    void loadUri(const std::string& uri) override {
        if (failed(this->m.media->LoadUri(uri.c_str()))) this->fireError("The service could not load " + uri);
    }
    void play() override { this->m.media->Play(); }
    void pause() override { this->m.media->Pause(); }
    void stop() override { this->m.media->Stop(); }
    void seek(int positionMs) override { this->m.media->Seek(positionMs); }

    int getPositionMs() const override {
        int32_t value = 0;
        return succeeded(this->m.media->GetPositionMs(&value)) ? value : 0;
    }
    int getDurationMs() const override {
        int32_t value = 0;
        return succeeded(this->m.media->GetDurationMs(&value)) ? value : 0;
    }

private:
    void detachListener() {
        if (!m_listener) return;
        this->m.media->SetPlaybackListener(nullptr);
        m_listener->detach();
        m_listener.reset();
    }

    void deliverState(int32_t state) override {
        if (state < kPlaybackIdle || state > kPlaybackError) return;
        this->setPlaybackState(static_cast<PlaybackState>(state));
    }
    void deliverPosition(int32_t positionMs, int32_t durationMs) override {
        this->firePositionChanged(positionMs, durationMs);
    }
    void deliverTrack(const std::string& title, const std::string& artist, const std::string& album) override {
        this->fireTrackChanged(title, artist, album);
    }
    void deliverError(const std::string& message) override { this->fireError(message); }
    void deliverCompleted() override { this->fireCompleted(); }

    ComPtr<HostPlaybackListener> m_listener;
};

class ComMusicService final : public ComMediaImpl<service::MusicService> {
public:
    using ComMediaImpl<service::MusicService>::ComMediaImpl;

    std::optional<Candidate> lookup(const std::string& isrc) override {
        ComPtr<IString> uri, title, artist;
        const Result r = m.music->Lookup(isrc.c_str(), uri.put(), title.put(), artist.put());
        return candidate(r, uri, title, artist, true);
    }

    std::optional<Candidate> lookupByTitleArtist(const std::string& title, const std::string& artist) override {
        ComPtr<IString> uri, foundTitle, foundArtist;
        const Result r = m.music->LookupByTitleArtist(title.c_str(), artist.c_str(), uri.put(),
                                                      foundTitle.put(), foundArtist.put());
        return candidate(r, uri, foundTitle, foundArtist, false);
    }

private:
    static std::optional<Candidate> candidate(Result r, const ComPtr<IString>& uri, const ComPtr<IString>& title,
                                              const ComPtr<IString>& artist, bool byIsrc) {
        if (r != kOk) return std::nullopt;
        Candidate c;
        c.uri = toStd(uri.get());
        c.title = toStd(title.get());
        c.artist = toStd(artist.get());
        c.byIsrc = byIsrc;
        return c;
    }
};

} // namespace

std::unique_ptr<service::Service> wrapService(ComPtr<IService> svc, Executor ui) {
    if (!svc) return nullptr;
    Core core;
    core.id = getString([&](IString** o) { return svc->GetId(o); });
    if (core.id.empty()) return nullptr;
    core.name = getString([&](IString** o) { return svc->GetName(o); });
    if (core.name.empty()) core.name = core.id;
    core.auth = svc.as<IAuthAspect>();
    core.media = svc.as<IMediaService>();
    core.music = svc.as<IMusicService>();
    core.ui = std::move(ui);
    core.service = std::move(svc);

    if (core.media && core.music) return std::make_unique<ComMusicService>(std::move(core));
    if (core.media) return std::make_unique<ComMediaImpl<service::MediaService>>(std::move(core));
    return std::make_unique<ComServiceImpl<service::Service>>(std::move(core));
}

} // namespace plugin
} // namespace space
