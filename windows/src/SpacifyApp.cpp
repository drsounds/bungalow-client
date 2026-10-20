#include <Windows.h>
#include <objbase.h>

#include "BungalowApp.h"
#include "PlayContext.h"
#include "SPElement.h"
#include "Database.h"
#include "services/LocalMediaService.h"
#include "models/SPPlayable.h"
#include "models/SPPlayableTrack.h"
#include "models/SPTrack.h"
#include "models/SPRecording.h"
#include "models/SPRecordingArtist.h"
#include "models/SPArtist.h"
#include "models/SPRelease.h"
#include "models/SPDBConnection.h"
#include "SPPlaybackEventArgs.h"

#include "SPColor.h"
#include "MainElement.h"
#include "SPMessageType.h"
#include "SPWindowElement.h"
#include "SPSkin.h"
#include "SiteView.h"
#include "Space.h"
#include "SPGraphicsContext.h"
#include "SPClassicSkin.h"
#include "SPModernSkin.h"
#include "SPFrutigerAeroSkin.h"
#include "ConfigView.h"
#include "UiDispatcher.h"
#include "PlatformUi.h"
#include "SPAppManager.h"
#include "SPUriStore.h"
#include "EntityResolvers.h"

#include <nlohmann/json.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>

BungalowApp* BungalowApp::s_instance = nullptr;

namespace {
    VOID CALLBACK positionTimerProc(HWND, UINT, UINT_PTR, DWORD) {
        if (auto* app = BungalowApp::current()) app->onPositionTimer();
    }
}

BungalowApp::BungalowApp() {
    s_instance = this;
}

BungalowApp::~BungalowApp() {
    shutdown();
    if (s_instance == this) s_instance = nullptr;
}

space::MainElement* BungalowApp::initialize(space::Space* spaceInstance, space::SPWindowElement* windowElement) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    space::win::initUiDispatcher();
    space::SPApplicationContext::setCurrent(this);

    m_space = spaceInstance;
    m_windowElement = windowElement;

    space::SPElement::s_xulErrorHandler = [this](const std::string& msg) {
        showError(msg);
    };

    m_mediaServices.push_back(new LocalMediaService());

    // The resources/ directory sits next to the executable or in an ancestor
    // directory (e.g. the repo root when running from build/ninja).
    wchar_t exePath[MAX_PATH * 2] = L"";
    GetModuleFileNameW(nullptr, exePath, static_cast<DWORD>(sizeof(exePath) / sizeof(exePath[0])));
    namespace fs = std::filesystem;
    fs::path exeDir = fs::path(exePath).parent_path();
    fs::path resourcesDir = exeDir;
    for (fs::path dir = exeDir; dir.has_parent_path(); dir = dir.parent_path()) {
        if (fs::exists(dir / "views" / "app.xml")) {
            resourcesDir = dir;
            break;
        }
        if (dir == dir.parent_path()) break;
    }
    resourcesDir = resourcesDir.lexically_normal();

    // Relative XUL paths resolve against the resources directory.
    SetCurrentDirectoryW(resourcesDir.c_str());

    // Discover app bundles (Apps/, ~/Bungalow[/Apps], ~/Documents/Bungalow[/Apps])
    // before creating the skin, which may come from one of them.
    auto& apps = space::SPAppManager::instance();
    apps.addDefaultSearchPaths(resourcesDir.u8string(), space::platform::homeDir(),
                               space::platform::documentsDir());
    apps.scan();

    // Use an existing bundled database, otherwise create one in the user data dir.
    fs::path dbPath = resourcesDir / "bungalow.db";
    if (!fs::exists(dbPath)) {
        fs::path dataDir = fs::u8path(space::platform::userDataDir());
        std::error_code ec;
        fs::create_directories(dataDir, ec);
        dbPath = dataDir / "bungalow.db";
    }
    m_databasePath = dbPath.generic_string();
    m_database = std::make_unique<Database>(m_databasePath);
    m_downloadManager = std::make_unique<DownloadManager>(m_database->handle());

    // Global URI entity store: views render the JSON-cached entity at once
    // and update when the resolver delivers fresh data.
    auto& uriStore = space::SPUriStore::instance();
    uriStore.setPersistenceDirectory(space::platform::userDataDir() + "/cache/uris");
    uriStore.setScheduler([](std::function<void()> task) { space::win::postToUi(std::move(task)); });
    space::registerEntityResolvers(uriStore, *m_database);

    m_main = new space::MainElement(m_windowElement);
    m_main->setId("main");

    space::SPColor tintColor(100, 127, 255, 127);
    short colorScheme          = SP_COLOR_SCHEME_DARK;
    bool  glossy               = false;
    std::string skinType       = "modern";
    bool  fullWidthAddressBar  = true;
    float aeroHue              = 200.0f;
    float aeroSaturation       = 0.0f;
    float aeroLightness        = 0.50f;
    int   aeroHeaderStyle      = static_cast<int>(space::kAeroHeaderGlossy);
    int   buttonStyle          = static_cast<int>(space::kButtonAqua);
    {
        std::ifstream settingsFile(space::views::ConfigView::getSettingsPath());
        if (settingsFile.is_open()) {
            std::ostringstream ss;
            ss << settingsFile.rdbuf();
            try {
                nlohmann::json obj = nlohmann::json::parse(ss.str());
                if (obj.contains("tintColor"))
                    tintColor = space::SPColor(obj.at("tintColor").get<std::string>());
                if (obj.contains("colorScheme"))
                    colorScheme = static_cast<short>(obj.at("colorScheme").get<std::int64_t>());
                if (obj.contains("glossy"))
                    glossy = obj.at("glossy").get<bool>();
                if (obj.contains("developerMode"))
                    m_developerMode = obj.at("developerMode").get<bool>();
                if (obj.contains("skinType"))
                    skinType = obj.at("skinType").get<std::string>();
                if (obj.contains("fullWidthAddressBar"))
                    fullWidthAddressBar = obj.at("fullWidthAddressBar").get<bool>();
                if (obj.contains("aeroHue"))
                    aeroHue = static_cast<float>(obj.at("aeroHue").get<double>());
                if (obj.contains("aeroSaturation"))
                    aeroSaturation = static_cast<float>(obj.at("aeroSaturation").get<double>());
                if (obj.contains("aeroLightness"))
                    aeroLightness = static_cast<float>(obj.at("aeroLightness").get<double>());
                if (obj.contains("aeroHeaderStyle"))
                    aeroHeaderStyle = static_cast<int>(obj.at("aeroHeaderStyle").get<std::int64_t>());
                if (obj.contains("buttonStyle"))
                    buttonStyle = static_cast<int>(obj.at("buttonStyle").get<std::int64_t>());
            } catch (...) {}
        }
    }

    space::SPSkin* activeSkin = nullptr;
    if (skinType == "frutiger-aero") {
        auto* aero = new space::SPFrutigerAeroSkin();
        aero->setHeaderStyle(static_cast<space::AeroHeaderStyle>(aeroHeaderStyle));
        aero->setGlossy(glossy);
        aero->setButtonStyle(static_cast<space::SPButtonStyle>(buttonStyle));
        activeSkin = aero;
    } else if (skinType == "classic") {
        activeSkin = new space::SPClassicSkin();
        activeSkin->setGlossy(glossy);
        activeSkin->setButtonStyle(static_cast<space::SPButtonStyle>(buttonStyle));
    } else {
        // "modern" and installed skin bundles are skin.xml skins; fall back to
        // procedural drawing if the bundle is missing.
        activeSkin = space::SPAppManager::instance().createSkin(skinType);
        if (!activeSkin) activeSkin = new space::SPModernSkin();
        activeSkin->setGlossy(glossy);
        activeSkin->setButtonStyle(static_cast<space::SPButtonStyle>(buttonStyle));
    }
    // The Now Playing colour sliders drive hue/saturation/lightness for every
    // skin, so restore them regardless of skin type.
    activeSkin->setHue(aeroHue);
    activeSkin->setSaturation(aeroSaturation);
    activeSkin->setLightness(aeroLightness);
    m_windowElement->setSkin(activeSkin);

    m_windowElement->setColorScheme(colorScheme);
    m_windowElement->setTintColor(space::SPColor::fromHSL(aeroHue, aeroSaturation, aeroLightness));
    m_main->setFullWidthAddressBar(fullWidthAddressBar);
    m_windowElement->appendChild(m_main);
    return m_main;
}

void BungalowApp::shutdown() {
    if (space::SPApplicationContext::current() == this)
        space::SPApplicationContext::setCurrent(nullptr);
    stopPositionTimer();
    for (SPMediaService* svc : m_mediaServices)
        delete svc;
    m_mediaServices.clear();
    for (SPPlayable* p : m_queuePlayables)
        delete p;
    m_queuePlayables.clear();
    m_queueEntries.clear();
    m_currentPlayable = nullptr;
}

void BungalowApp::startPositionTimer() {
    if (m_timerId) return;
    m_timerId = SetTimer(nullptr, 0, 500, positionTimerProc);
}

void BungalowApp::stopPositionTimer() {
    if (!m_timerId) return;
    KillTimer(nullptr, static_cast<UINT_PTR>(m_timerId));
    m_timerId = 0;
}

std::string BungalowApp::currentPlayableUri() const {
    return m_currentPlayable ? m_currentPlayable->uri : std::string();
}

SPMediaService* BungalowApp::getMediaServiceForUri(const std::string& uri) const {
    for (SPMediaService* svc : m_mediaServices) {
        if (svc && svc->acceptsUri(uri))
            return svc;
    }
    return nullptr;
}

void BungalowApp::navigate(const std::string& uri) {
    if (m_main) m_main->navigate(uri);
}

void BungalowApp::showError(const std::string& msg) {
    if (m_main) m_main->showMessage(space::Error, msg);
}

static std::string uriBasename(const std::string& uri) {
    const auto slash = uri.find_last_of("/\\");
    std::string name = (slash == std::string::npos) ? uri : uri.substr(slash + 1);
    const auto dot = name.rfind('.');
    if (dot != std::string::npos) name = name.substr(0, dot);
    return name;
}

void BungalowApp::stopCurrentPlayback() {
    stopPositionTimer();
    if (m_currentPlayable && m_currentPlayable->service)
        m_currentPlayable->service->stop();
    m_currentPlayable = nullptr;
    m_isPlaying       = false;
    m_positionMs      = 0;
}

void BungalowApp::playContext(PlayContext ctx) {
    stopCurrentPlayback();

    for (SPPlayable* p : m_queuePlayables)
        delete p;
    m_queuePlayables.clear();
    m_queueEntries.clear();
    m_queueIndex = -1;

    for (auto& entry : ctx.entries) {
        SPPlayable* p = nullptr;
        if (entry.playable && !entry.uri.empty()) {
            p = new SPPlayable(entry.uri);
            p->resolveService(m_mediaServices);
            if (!p->service) {
                entry.playable = false;
                delete p;
                p = nullptr;
            }
        }
        m_queuePlayables.push_back(p);
    }
    m_queueEntries = std::move(ctx.entries);

    m_playbackEvents.dispatchEvent("queue:changed", nullptr, nullptr);

    playFromIndex(static_cast<int>(ctx.startIndex));
}

void BungalowApp::playFromIndex(int idx) {
    const int size = static_cast<int>(m_queueEntries.size());

    while (idx < size && !m_queueEntries[idx].playable)
        ++idx;

    if (idx >= size) {
        m_isPlaying = false;
        dispatchPlaybackChanged();
        return;
    }

    m_queueIndex = idx;
    SPPlayable*       p     = m_queuePlayables[idx];
    const PlayQueueEntry& entry = m_queueEntries[idx];

    if (!p || !p->service) {
        m_isPlaying = false;
        dispatchPlaybackChanged();
        return;
    }

    m_trackTitle   = entry.displayTitle.empty() ? uriBasename(entry.uri) : entry.displayTitle;
    m_artistName   = entry.displayArtist;
    m_coverArtPath = entry.coverArtPath;

    p->service->load(entry.uri);
    p->service->play();
    m_currentPlayable = p;
    m_isPlaying       = true;
    m_positionMs      = 0;
    m_durationMs      = p->service->getDurationMs();

    dispatchPlaybackChanged();
    startPositionTimer();
}

void BungalowApp::playQueueAtIndex(int idx) {
    if (idx < 0 || idx >= static_cast<int>(m_queueEntries.size())) return;
    if (!m_queueEntries[idx].playable) return;

    stopCurrentPlayback();
    playFromIndex(idx);
}

void BungalowApp::playTrackId(int64_t trackId) {
    if (!m_database) return;
    SPDBConnection conn(*m_database);

    auto ptRows = SPPlayableTrack::findByTrack(conn, trackId);
    if (ptRows.empty()) { showError("An error occurred"); return; }

    auto playableOpt = SPPlayable::find(conn, ptRows.front().playableId);
    if (!playableOpt) { showError("An error occurred"); return; }

    std::string title, artist, coverArt;
    auto trackOpt = SPTrack::find(conn, trackId);
    if (trackOpt) {
        auto recOpt = SPRecording::find(conn, trackOpt->recordingId);
        if (recOpt) {
            title = recOpt->name;
            if (!recOpt->version.empty())
                title += " (" + recOpt->version + ")";
        }
        auto raRows = SPRecordingArtist::findByRecording(conn, trackOpt->recordingId);
        for (const auto& ra : raRows) {
            if (ra.isPrimaryArtist || artist.empty()) {
                auto artOpt = SPArtist::find(conn, ra.artistId);
                if (artOpt) artist = artOpt->name;
                if (ra.isPrimaryArtist) break;
            }
        }
        if (!trackOpt->releaseId.empty()) {
            auto relOpt = SPRelease::find(conn, trackOpt->releaseId);
            if (relOpt) coverArt = relOpt->coverArt;
        }
    }

    PlayQueueEntry e;
    e.displayTitle  = title;
    e.displayArtist = artist;
    e.coverArtPath  = coverArt;
    e.uri           = playableOpt->uri;
    e.playable      = true;

    PlayContext ctx;
    ctx.entries.push_back(std::move(e));
    ctx.startIndex = 0;
    playContext(std::move(ctx));
}

void BungalowApp::playUri(const std::string& uri) {
    PlayQueueEntry e;
    e.displayTitle = uriBasename(uri);
    e.uri          = uri;
    e.playable     = true;

    PlayContext ctx;
    ctx.entries.push_back(std::move(e));
    ctx.startIndex = 0;
    playContext(std::move(ctx));
}

void BungalowApp::togglePlayPause() {
    if (!m_currentPlayable) return;

    m_isPlaying = !m_isPlaying;
    if (m_currentPlayable->service) {
        if (m_isPlaying)
            m_currentPlayable->service->play();
        else
            m_currentPlayable->service->pause();
    }

    if (m_isPlaying)
        startPositionTimer();
    else
        stopPositionTimer();

    dispatchPlaybackChanged();
}

void BungalowApp::seekTo(int posMs) {
    if (!m_currentPlayable) return;

    m_positionMs = posMs;
    if (m_currentPlayable->service)
        m_currentPlayable->service->seek(posMs);

    dispatchPlaybackChanged();
}

void BungalowApp::playbackNext() {
    stopCurrentPlayback();

    const int next = m_queueIndex + 1;
    if (next < static_cast<int>(m_queueEntries.size()))
        playFromIndex(next);
    else
        dispatchPlaybackChanged();
}

void BungalowApp::playbackPrev() {
    if (m_positionMs > 3000) {
        seekTo(0);
        return;
    }

    if (m_queueIndex < 0) return;

    stopCurrentPlayback();

    int prev = m_queueIndex - 1;
    while (prev >= 0 && !m_queueEntries[prev].playable)
        --prev;

    if (prev >= 0)
        playFromIndex(prev);
    else
        playFromIndex(m_queueIndex);
}

void BungalowApp::dispatchPlaybackChanged() {
    auto* args = new space::SPPlaybackStateEventArgs(
        m_isPlaying, m_positionMs, m_durationMs);
    m_playbackEvents.dispatchEvent("playback:changed", args, nullptr);
}

void BungalowApp::onPositionTimer() {
    if (!m_isPlaying || !m_currentPlayable) return;

    SPMediaService* svc = m_currentPlayable->service;

    if (svc) {
        m_positionMs = svc->getPositionMs();
        if (m_durationMs == 0)
            m_durationMs = svc->getDurationMs();
    } else {
        m_positionMs += 500;
    }

    if (m_durationMs > 0 && m_positionMs >= m_durationMs) {
        m_positionMs = m_durationMs;
        playbackNext();
        return;
    }

    dispatchPlaybackChanged();
}

void BungalowApp::setDeveloperMode(bool enabled) {
    m_developerMode = enabled;
}

void BungalowApp::setFullWidthAddressBar(bool fullWidth) {
    if (!m_main) return;
    m_main->setFullWidthAddressBar(fullWidth);
    if (m_windowElement) {
        m_windowElement->pack();
        auto* gc = m_windowElement->createGraphics();
        m_windowElement->invalidateRegion({0, 0, gc->getWidth(), gc->getHeight()});
    }
}

void BungalowApp::showWebInspector() {
    if (auto* sv = space::views::SiteView::getActiveInstance())
        sv->showInspector();
}

void BungalowApp::reloadWebPage() {
    if (auto* sv = space::views::SiteView::getActiveInstance())
        sv->reload();
}
