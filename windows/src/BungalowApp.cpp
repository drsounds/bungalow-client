#include <Windows.h>
#include <objbase.h>

#include "BungalowApp.h"
#include "PlayContext.h"
#include "SPElement.h"
#include "Database.h"
#include "service/LocalMediaService.h"
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
#include "ui/MainWindow.h"
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
#include "SPDropdownElement.h"
#include "SPUriStore.h"
#include "EntityResolvers.h"
#ifdef BUNGALOW_SWIMMING
#include "SwimmingScheduler.h"
#endif

#include <nlohmann/json.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>

BungalowApp* BungalowApp::s_instance = nullptr;

using space::service::MediaService;
using space::service::PlaybackCoordinator;
using space::service::PlayRequest;
using space::service::ServiceMatch;

namespace {
    VOID CALLBACK positionTimerProc(HWND, UINT, UINT_PTR, DWORD) {
        if (auto* app = BungalowApp::current()) app->onPositionTimer();
    }

    // The "Play with…" chooser: a popup menu listing every match, once to
    // play it now and once to always play the track with it.
    std::optional<PlaybackCoordinator::Choice> chooseServiceMatch(const PlayRequest&,
                                                                  const std::vector<ServiceMatch>& matches) {
        const int count = static_cast<int>(matches.size());
        space::platform::MenuItems items;
        for (int i = 0; i < count; ++i)
            items.emplace_back(i + 1, "Play with " + matches[i].label()); // id 0 is the separator
        items.emplace_back(0, "");
        for (int i = 0; i < count; ++i)
            items.emplace_back(count + i + 1, "Always play with " + matches[i].label());

        const int id = space::platform::showPopupMenu(items);
        if (id <= 0) return std::nullopt;
        if (id <= count) return PlaybackCoordinator::Choice{static_cast<std::size_t>(id - 1), false};
        return PlaybackCoordinator::Choice{static_cast<std::size_t>(id - count - 1), true};
    }
}

BungalowApp::BungalowApp() {
    s_instance = this;
}

BungalowApp::~BungalowApp() {
    shutdown();
    if (s_instance == this) s_instance = nullptr;
}

space::ui::MainWindow* BungalowApp::initialize(space::Space* spaceInstance, space::SPWindowElement* windowElement) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    space::win::initUiDispatcher();
    space::SPApplicationContext::setCurrent(this);

    m_space = spaceInstance;
    m_windowElement = windowElement;

    space::SPElement::s_xulErrorHandler = [this](const std::string& msg) {
        showError(msg);
    };
    space::SPDropdownElement::s_showMenu = [](const std::vector<std::string>& labels) {
        space::platform::MenuItems items;
        for (size_t i = 0; i < labels.size(); ++i)
            items.emplace_back(static_cast<int>(i) + 1, labels[i]); // id 0 means "dismissed"
        const int id = space::platform::showPopupMenu(items);
        return id > 0 ? id - 1 : -1;
    };
#ifdef BUNGALOW_SWIMMING
    // Swimming apps' JS timers and repaints
    space::win::installSwimmingScheduler();
#endif

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

    // Services, selected by capability; each track is resolved to one at play time.
    m_resolutionStore = std::make_unique<space::service::ResolutionStore>(m_database->handle());
    m_coordinator = std::make_unique<PlaybackCoordinator>(m_services, m_resolutionStore.get());
    m_coordinator->setChooser(chooseServiceMatch);
    m_services.registerService(std::make_unique<space::service::LocalMediaService>(m_database.get()));
    m_services.startAll();

    // Global URI entity store: views render the JSON-cached entity at once
    // and update when the resolver delivers fresh data.
    auto& uriStore = space::SPUriStore::instance();
    uriStore.setPersistenceDirectory(space::platform::userDataDir() + "/cache/uris");
    uriStore.setScheduler([](std::function<void()> task) { space::win::postToUi(std::move(task)); });
    space::registerEntityResolvers(uriStore, *m_database);

    m_main = new space::ui::MainWindow(m_windowElement);
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
        // "modern" is the procedural skin; anything else is a skin.xml bundle,
        // falling back to the procedural skin if the bundle is missing.
        if (skinType != "modern")
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
    stopCurrentPlayback();
    m_coordinator.reset();
    m_services.shutdownAll();
    m_resolutionStore.reset();
    m_queueEntries.clear();
    m_queueIndex = -1;
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

// The entry's own URI, which is what views compare their rows against, even
// when the entry resolved to another service.
std::string BungalowApp::currentPlayableUri() const {
    if (!m_hasCurrent || m_queueIndex < 0 || m_queueIndex >= static_cast<int>(m_queueEntries.size()))
        return {};
    return m_queueEntries[m_queueIndex].uri;
}

MediaService* BungalowApp::getActiveService() const {
    return m_coordinator ? m_coordinator->getActiveService() : nullptr;
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

// bungalow:track:<id> names a track of the library.
static bool parseTrackUri(const std::string& uri, int64_t& trackId) {
    static const std::string kPrefix = "bungalow:track:";
    if (uri.rfind(kPrefix, 0) != 0) return false;
    const std::string id = uri.substr(kPrefix.size());
    if (id.empty() || id.size() > 18) return false;
    for (char c : id)
        if (c < '0' || c > '9') return false;
    trackId = std::stoll(id);
    return true;
}

void BungalowApp::stopCurrentPlayback() {
    stopPositionTimer();
    if (m_hasCurrent) {
        if (MediaService* svc = getActiveService())
            svc->stop();
    }
    m_hasCurrent = false;
    m_isPlaying  = false;
    m_positionMs = 0;
}

// Worth trying: the entry has its own URI some service plays, or an ISRC the
// music services can resolve.
bool BungalowApp::canResolve(const PlayQueueEntry& entry) const {
    if (!m_coordinator) return false;
    if (!entry.uri.empty() && m_coordinator->serviceForUri(entry.uri)) return true;
    return !entry.isrc.empty();
}

void BungalowApp::playContext(PlayContext ctx) {
    stopCurrentPlayback();

    for (auto& entry : ctx.entries) {
        if (entry.playable && !canResolve(entry))
            entry.playable = false;
    }
    m_queueEntries = std::move(ctx.entries);
    m_queueIndex   = -1;

    m_playbackEvents.dispatchEvent("queue:changed", nullptr, nullptr);

    playFromIndex(static_cast<int>(ctx.startIndex), ctx.chooseService);
}

void BungalowApp::playFromIndex(int idx, bool chooseService) {
    const int size = static_cast<int>(m_queueEntries.size());

    while (idx < size && !m_queueEntries[idx].playable)
        ++idx;

    if (idx >= size || !m_coordinator) {
        m_isPlaying = false;
        dispatchPlaybackChanged();
        return;
    }

    m_queueIndex = idx;
    const PlayQueueEntry& entry = m_queueEntries[idx];
    const std::string title = entry.displayTitle.empty() ? uriBasename(entry.uri) : entry.displayTitle;

    PlayRequest request;
    request.trackId     = entry.trackId;
    request.isrc        = entry.isrc;
    request.title       = title;
    request.artist      = entry.displayArtist;
    request.fallbackUri = entry.uri;

    const auto result = m_coordinator->resolveAndPlay(request, chooseService);
    if (!result.played()) {
        m_hasCurrent = false;
        m_isPlaying  = false;
        if (result.status == PlaybackCoordinator::Status::NotFound)
            showError("No installed service can play \"" + title + "\".");
        else if (result.status == PlaybackCoordinator::Status::Failed)
            showError("Couldn't play \"" + title + "\".");
        dispatchPlaybackChanged();
        return;
    }

    m_trackTitle   = title;
    m_artistName   = entry.displayArtist;
    m_coverArtPath = entry.coverArtPath;

    m_hasCurrent = true;
    m_isPlaying  = true;
    m_positionMs = 0;
    m_durationMs = result.service->getDurationMs();

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
    e.trackId       = trackId;
    if (trackOpt) e.isrc = trackOpt->recordingId; // recordings are keyed by ISRC

    PlayContext ctx;
    ctx.entries.push_back(std::move(e));
    ctx.startIndex = 0;
    playContext(std::move(ctx));
}

void BungalowApp::playUri(const std::string& uri) {
    int64_t trackId = 0;
    if (parseTrackUri(uri, trackId)) {
        playTrackId(trackId);
        return;
    }

    PlayQueueEntry e;
    e.playable = true;
    if (auto request = PlaybackCoordinator::requestForContentUri(uri)) {
        // A recording, not a source: resolved across the music services.
        e.isrc         = request->isrc;
        e.displayTitle = request->isrc;
    } else {
        e.displayTitle = uriBasename(uri);
        e.uri          = uri;
    }

    PlayContext ctx;
    ctx.entries.push_back(std::move(e));
    ctx.startIndex = 0;
    playContext(std::move(ctx));
}

bool BungalowApp::canPlayUri(const std::string& uri) const {
    int64_t trackId = 0;
    return parseTrackUri(uri, trackId) || (m_coordinator && m_coordinator->canPlayUri(uri));
}

void BungalowApp::togglePlayPause() {
    if (!m_hasCurrent) return;

    m_isPlaying = !m_isPlaying;
    if (MediaService* svc = getActiveService()) {
        if (m_isPlaying)
            svc->play();
        else
            svc->pause();
    }

    if (m_isPlaying)
        startPositionTimer();
    else
        stopPositionTimer();

    dispatchPlaybackChanged();
}

void BungalowApp::seekTo(int posMs) {
    if (!m_hasCurrent) return;

    m_positionMs = posMs;
    if (MediaService* svc = getActiveService())
        svc->seek(posMs);

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
    if (!m_isPlaying || !m_hasCurrent) return;

    if (MediaService* svc = getActiveService()) {
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
