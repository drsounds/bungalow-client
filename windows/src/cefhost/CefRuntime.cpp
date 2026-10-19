#include "cefhost/CefRuntime.h"
#include "CefRuntimeInternal.h"

#include "PlatformUi.h"

#include "include/cef_app.h"
#include "include/cef_v8.h"

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <map>
#include <mutex>
#include <set>
#include <vector>

namespace fs = std::filesystem;

namespace space {
namespace cefhost {
namespace {

// Defines window.navigate(path) for apps that do not bring their own: the host
// calls it to route the running app (spacify:app:<id>:a:b -> "/a/b").
constexpr char kBridgeScript[] = R"js(
if (typeof window.navigate !== 'function') {
    window.navigate = function (path) {
        if (location.pathname + location.search + location.hash === path) return;
        history.pushState(null, '', path);
        window.dispatchEvent(new PopStateEvent('popstate', { state: null }));
    };
}
)js";

// Implements window.spacify.navigate(uri) in the renderer.
class NavigateHandler : public CefV8Handler {
public:
    bool Execute(const CefString&, CefRefPtr<CefV8Value>, const CefV8ValueList& arguments,
                 CefRefPtr<CefV8Value>&, CefString& exception) override {
        if (arguments.size() != 1 || !arguments[0]->IsString()) {
            exception = "spacify.navigate(uri) expects a spacify: URI string";
            return true;
        }
        CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(kNavigateMessage);
        message->GetArgumentList()->SetString(0, arguments[0]->GetStringValue());
        CefV8Context::GetCurrentContext()->GetFrame()->SendProcessMessage(PID_BROWSER, message);
        return true;
    }

private:
    IMPLEMENT_REFCOUNTING(NavigateHandler);
};

// The CefApp for every process: registers sp:// everywhere and installs the
// JavaScript bridge in app renderers.
class BungalowCefApp : public CefApp, public CefRenderProcessHandler {
public:
    CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override { return this; }

    void OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar) override {
        registrar->AddCustomScheme(kAppScheme,
            CEF_SCHEME_OPTION_STANDARD | CEF_SCHEME_OPTION_SECURE |
            CEF_SCHEME_OPTION_CORS_ENABLED | CEF_SCHEME_OPTION_FETCH_ENABLED);
    }

    // ── Renderer process ──────────────────────────────────────────────────

    void OnBrowserCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDictionaryValue> extraInfo) override {
        if (extraInfo && extraInfo->HasKey(kStartupScriptKey))
            m_startupScripts[browser->GetIdentifier()] = {
                extraInfo->GetString(kStartupScriptKey),
                extraInfo->HasKey(kSiteBrowserKey) && extraInfo->GetBool(kSiteBrowserKey)};
    }

    void OnBrowserDestroyed(CefRefPtr<CefBrowser> browser) override {
        m_startupScripts.erase(browser->GetIdentifier());
    }

    void OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                          CefRefPtr<CefV8Context> context) override {
        auto script = m_startupScripts.find(browser->GetIdentifier());
        if (script == m_startupScripts.end()) return;
        const std::string url = frame->GetURL();

        CefRefPtr<CefV8Value> result;
        CefRefPtr<CefV8Exception> error;
        if (script->second.siteBrowser) {
            if (frame->IsMain() && !script->second.script.empty())
                context->Eval(script->second.script, url, 0, result, error);
            return;
        }
        if (url.rfind(std::string(kAppScheme) + "://", 0) != 0) return;

        CefRefPtr<CefV8Value> spacify = CefV8Value::CreateObject(nullptr, nullptr);
        spacify->SetValue("navigate", CefV8Value::CreateFunction("navigate", new NavigateHandler()),
                          V8_PROPERTY_ATTRIBUTE_READONLY);
        context->GetGlobal()->SetValue("spacify", spacify, V8_PROPERTY_ATTRIBUTE_READONLY);

        context->Eval(std::string(kBridgeScript) + script->second.script.ToString(), url, 0, result, error);
    }

private:
    struct StartupScript {
        CefString script;
        bool siteBrowser = false;
    };

    // Browser id -> startup script. Renderer main thread only.
    std::map<int, StartupScript> m_startupScripts;

    IMPLEMENT_REFCOUNTING(BungalowCefApp);
};

CefRefPtr<BungalowCefApp> g_app;
bool g_initialized = false;

std::mutex g_mutex;
std::condition_variable g_changed;
std::vector<CefRefPtr<CefBrowser>> g_browsers;
int g_pendingBrowsers = 0;
bool g_shuttingDown = false;

CefRefPtr<BungalowCefApp> app() {
    if (!g_app) g_app = new BungalowCefApp();
    return g_app;
}

} // namespace

int executeSubprocess(HINSTANCE instance) {
    CefMainArgs args(instance);
    return CefExecuteProcess(args, app(), nullptr);
}

std::wstring cacheRoot() {
    // Normalized: CEF compares profile paths against this one textually.
    return (fs::path(reinterpret_cast<const char8_t*>(platform::userDataDir().c_str())) / L"cef")
        .lexically_normal().make_preferred().wstring();
}

bool initialize(HINSTANCE instance) {
    const fs::path root = cacheRoot();
    std::error_code ec;
    fs::create_directories(root, ec);

    CefSettings settings;
    settings.no_sandbox = true;
    // CEF runs its own UI thread, so the WTL message loop stays untouched.
    settings.multi_threaded_message_loop = true;
    settings.log_severity = LOGSEVERITY_WARNING;
    CefString(&settings.root_cache_path) = root.wstring();
    CefString(&settings.log_file) = (root / L"cef.log").wstring();

    CefMainArgs args(instance);
    g_initialized = CefInitialize(args, settings, app(), nullptr);
    return g_initialized;
}

bool isInitialized() {
    return g_initialized;
}

bool beginBrowserCreation() {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_initialized || g_shuttingDown) return false;
    ++g_pendingBrowsers;
    return true;
}

void abortBrowserCreation() {
    std::lock_guard<std::mutex> lock(g_mutex);
    --g_pendingBrowsers;
    g_changed.notify_all();
}

void browserCreated(CefRefPtr<CefBrowser> browser) {
    bool shuttingDown;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        --g_pendingBrowsers;
        g_browsers.push_back(browser);
        shuttingDown = g_shuttingDown;
        g_changed.notify_all();
    }
    // Outside the lock: CloseBrowser may call browserClosed() synchronously.
    if (shuttingDown) browser->GetHost()->CloseBrowser(true);
}

void browserClosed(CefRefPtr<CefBrowser> browser) {
    std::lock_guard<std::mutex> lock(g_mutex);
    std::erase_if(g_browsers, [&](const CefRefPtr<CefBrowser>& b) { return b->IsSame(browser); });
    g_changed.notify_all();
}

void shutdown() {
    if (!g_initialized) return;
    closeAllBrowsers();
    CefShutdown();
    g_initialized = false;
    g_app = nullptr;
}

void closeAllBrowsers() {
    std::vector<CefRefPtr<CefBrowser>> browsers;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_shuttingDown = true;
        browsers = g_browsers;
    }
    // Usually already closing: destroying the frame destroyed their windows.
    for (const auto& browser : browsers)
        browser->GetHost()->CloseBrowser(true);

    // Browser windows are destroyed on the CEF thread but may still message
    // this thread, so keep pumping while waiting. The main loop has already
    // exited, so no WM_QUIT can be swallowed here.
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    for (;;) {
        {
            std::unique_lock<std::mutex> lock(g_mutex);
            if (g_browsers.empty() && g_pendingBrowsers == 0) return;
            if (std::chrono::steady_clock::now() >= deadline) return;
            g_changed.wait_for(lock, std::chrono::milliseconds(10));
        }
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
}

} // namespace cefhost
} // namespace space
