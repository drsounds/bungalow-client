// MCI-based implementation of the sp_audio_* bridge used by LocalMediaService.
#include <Windows.h>
#include <mmsystem.h>

#include <string>

namespace {
    const wchar_t* kAlias = L"bungalow_audio";
    bool g_open = false;
    bool g_playing = false;

    std::wstring widen(const std::string& s) {
        if (s.empty()) return {};
        int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
        std::wstring w(n > 0 ? n - 1 : 0, L'\0');
        if (n > 0) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, w.data(), n);
        return w;
    }

    bool mci(const std::wstring& cmd, std::wstring* reply = nullptr) {
        wchar_t buf[128] = L"";
        const MCIERROR err = mciSendStringW(cmd.c_str(), buf, 128, nullptr);
        if (reply) *reply = buf;
        return err == 0;
    }

    void closeCurrent() {
        if (g_open) {
            mci(std::wstring(L"close ") + kAlias);
            g_open = false;
        }
        g_playing = false;
    }

    double queryMs(const wchar_t* what) {
        if (!g_open) return 0.0;
        std::wstring reply;
        if (!mci(std::wstring(L"status ") + kAlias + L" " + what, &reply) || reply.empty()) return 0.0;
        return _wtof(reply.c_str());
    }
}

extern "C" {

bool sp_audio_load(const char* path) {
    closeCurrent();
    std::string p = path ? path : "";
    // "file:///C:/x" arrives as "/C:/x"
    if (p.size() > 2 && p[0] == '/' && p[2] == ':') p.erase(0, 1);
    for (char& c : p) if (c == '/') c = '\\';

    std::wstring cmd = L"open \"" + widen(p) + L"\" type mpegvideo alias " + kAlias;
    if (!mci(cmd)) {
        cmd = L"open \"" + widen(p) + L"\" alias " + kAlias;
        if (!mci(cmd)) return false;
    }
    mci(std::wstring(L"set ") + kAlias + L" time format milliseconds");
    g_open = true;
    return true;
}

void sp_audio_play() {
    if (!g_open) return;
    if (mci(std::wstring(L"play ") + kAlias)) g_playing = true;
}

void sp_audio_pause() {
    if (!g_open) return;
    mci(std::wstring(L"pause ") + kAlias);
    g_playing = false;
}

void sp_audio_stop() {
    closeCurrent();
}

void sp_audio_seek(double seconds) {
    if (!g_open) return;
    const long long ms = static_cast<long long>(seconds * 1000.0);
    const std::wstring target = std::to_wstring(ms);
    if (g_playing)
        mci(std::wstring(L"play ") + kAlias + L" from " + target);
    else
        mci(std::wstring(L"seek ") + kAlias + L" to " + target);
}

double sp_audio_get_duration() {
    return queryMs(L"length") / 1000.0;
}

double sp_audio_get_position() {
    return queryMs(L"position") / 1000.0;
}

bool sp_audio_is_playing() {
    if (!g_open) return false;
    std::wstring reply;
    if (!mci(std::wstring(L"status ") + kAlias + L" mode", &reply)) return false;
    return reply == L"playing";
}

// MCI offers no sample access, so visualizers receive silence.
int sp_audio_get_pcm(float*, int) { return 0; }
int sp_audio_get_fft(float*, int) { return 0; }

}
