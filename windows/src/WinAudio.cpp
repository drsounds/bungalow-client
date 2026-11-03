// Media Foundation (MFPlay) implementation of the sp_audio_* bridge used by
// LocalMediaService.
//
// This replaces an MCI "mpegvideo" backend: on current Windows the MCI MPEG
// driver fails to initialise for real-world MP3s ("problem initializing MCI"),
// so load() silently failed and the player showed a track that never played.
// Media Foundation plays MP3, AAC/M4A, FLAC, WAV and WMA natively.
//
// MFPlay delivers its events on the thread that created the player, through a
// hidden window, so the thread that calls sp_audio_load() must pump messages
// (the UI thread does).
#include <Windows.h>
#include <mfapi.h>
#include <mferror.h>
#include <mfplay.h>

#include <string>

namespace {

    std::wstring widen(const std::string& s) {
        if (s.empty()) return {};
        int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
        std::wstring w(n > 0 ? n - 1 : 0, L'\0');
        if (n > 0) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), n);
        return w;
    }

    IMFPMediaPlayer* g_player = nullptr;
    bool g_started   = false;  // MFStartup succeeded
    bool g_ready     = false;  // media item set; transport calls are valid
    bool g_wantPlay  = false;  // play() requested before the item was ready

    // Receives the player's asynchronous events.
    class Callback : public IMFPMediaPlayerCallback {
    public:
        STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
            if (riid == IID_IUnknown || riid == __uuidof(IMFPMediaPlayerCallback)) {
                *ppv = static_cast<IMFPMediaPlayerCallback*>(this);
                AddRef();
                return S_OK;
            }
            *ppv = nullptr;
            return E_NOINTERFACE;
        }
        STDMETHODIMP_(ULONG) AddRef() override { return 2; }   // static lifetime
        STDMETHODIMP_(ULONG) Release() override { return 1; }

        void STDMETHODCALLTYPE OnMediaPlayerEvent(MFP_EVENT_HEADER* header) override {
            // Ignore events of a player that has since been replaced.
            if (!header || header->pMediaPlayer != g_player) return;
            if (FAILED(header->hrEvent)) {
                g_ready = false;
                g_wantPlay = false;
                return;
            }
            if (header->eEventType == MFP_EVENT_TYPE_MEDIAITEM_SET) {
                g_ready = true;
                if (g_wantPlay) g_player->Play();
            }
        }
    };
    Callback g_callback;

    void ensureStarted() {
        if (g_started) return;
        // The UI thread normally has COM already; a different apartment model is fine.
        const HRESULT co = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        (void)co;
        g_started = SUCCEEDED(MFStartup(MF_VERSION));
    }

    void closeCurrent() {
        if (g_player) {
            g_player->Shutdown();
            g_player->Release();
            g_player = nullptr;
        }
        g_ready = false;
        g_wantPlay = false;
    }

    // Position or duration in seconds; 0 until the item is ready.
    double seconds(bool duration) {
        if (!g_player || !g_ready) return 0.0;
        PROPVARIANT pv;
        PropVariantInit(&pv);
        HRESULT hr = duration ? g_player->GetDuration(MFP_POSITIONTYPE_100NS, &pv)
                              : g_player->GetPosition(MFP_POSITIONTYPE_100NS, &pv);
        double out = 0.0;
        if (SUCCEEDED(hr) && (pv.vt == VT_I8 || pv.vt == VT_UI8))   // duration arrives as VT_UI8
            out = static_cast<double>(pv.hVal.QuadPart) / 1e7;
        PropVariantClear(&pv);
        return out;
    }
}

extern "C" {

bool sp_audio_load(const char* path) {
    closeCurrent();
    ensureStarted();
    if (!g_started) return false;

    std::string p = path ? path : "";
    // "file:///C:/x" arrives as "/C:/x"
    if (p.size() > 2 && p[0] == '/' && p[2] == ':') p.erase(0, 1);
    for (char& c : p) if (c == '/') c = '\\';

    // Not started: the item loads asynchronously and play() starts it.
    return SUCCEEDED(MFPCreateMediaPlayer(widen(p).c_str(), FALSE, 0, &g_callback, nullptr, &g_player));
}

void sp_audio_play() {
    if (!g_player) return;
    g_wantPlay = true;
    if (g_ready) g_player->Play();
}

void sp_audio_pause() {
    if (!g_player) return;
    g_wantPlay = false;
    if (g_ready) g_player->Pause();
}

void sp_audio_stop() {
    closeCurrent();
}

void sp_audio_seek(double secondsPos) {
    if (!g_player || !g_ready) return;
    PROPVARIANT pv;
    PropVariantInit(&pv);
    pv.vt = VT_I8;
    pv.hVal.QuadPart = static_cast<LONGLONG>(secondsPos * 1e7);
    g_player->SetPosition(MFP_POSITIONTYPE_100NS, &pv);
    PropVariantClear(&pv);
}

double sp_audio_get_duration() {
    return seconds(true);
}

double sp_audio_get_position() {
    return seconds(false);
}

bool sp_audio_is_playing() {
    if (!g_player || !g_ready) return false;
    MFP_MEDIAPLAYER_STATE state = MFP_MEDIAPLAYER_STATE_EMPTY;
    return SUCCEEDED(g_player->GetState(&state)) && state == MFP_MEDIAPLAYER_STATE_PLAYING;
}

// MFPlay offers no sample access, so visualizers receive silence.
int sp_audio_get_pcm(float*, int) { return 0; }
int sp_audio_get_fft(float*, int) { return 0; }

}
