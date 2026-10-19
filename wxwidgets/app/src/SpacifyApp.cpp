//
// Created by Alexander Forselius on 2025-12-03.
//

#include "SpacifyApp.h"
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
#include "SpaceFrame.h"
#include "SiteView.h"
#include "Space.h"
#include "SpaceWidget.h"
#include "SPClassicSkin.h"
#include "SPModernSkin.h"
#include "SPFrutigerAeroSkin.h"
#include "WxWidgetsGraphicsContext.h"
#include "ConfigView.h"
#include "PlatformUi.h"
#include "SPAppManager.h"

#include <nlohmann/json.hpp>
#include <cstdint>

#include <fstream>
#include <sstream>
#include <wx/stdpaths.h>
#include <wx/filename.h>

wxIMPLEMENT_APP(SpacifyApp);

// ── Position-timer event table ────────────────────────────────────────────────
static const int ID_POSITION_TIMER = wxNewId();

wxBEGIN_EVENT_TABLE(SpacifyApp, wxApp)
    EVT_TIMER(ID_POSITION_TIMER, SpacifyApp::onPositionTimer)
wxEND_EVENT_TABLE()

// ── Initialisation ────────────────────────────────────────────────────────────

bool SpacifyApp::OnInit(){
    if( !wxApp::OnInit() )
        return false;
    space::SPApplicationContext::setCurrent(this);

    // Route XUL load failures to the global info banner
    space::SPElement::s_xulErrorHandler = [this](const std::string& msg) {
        showError(msg);
    };

    // Register media services (searched in order by acceptsUri)
    m_mediaServices.push_back(new LocalMediaService());

    // Resolve the resources/ directory: it sits next to the executable, or
    // one level up when the binary lives in a build subdirectory.
    wxFileName exe(wxStandardPaths::Get().GetExecutablePath());
    wxFileName resourcesDir;
    resourcesDir.AssignDir(exe.GetPath() + "/resources");
    resourcesDir.Normalize(wxPATH_NORM_ALL);
    if (!resourcesDir.DirExists()) {
        resourcesDir.AssignDir(exe.GetPath() + "/../resources");
        resourcesDir.Normalize(wxPATH_NORM_ALL);
    }

    // Set CWD to the resources parent so relative XUL paths work from anywhere.
    wxSetWorkingDirectory(resourcesDir.GetPathWithSep() + "..");

    // Discover app bundles (Apps/, ~/Bungalow/Apps, ~/Documents/Bungalow/Apps)
    // before creating the skin, which may come from one of them.
    auto& apps = space::SPAppManager::instance();
    apps.addDefaultSearchPaths(std::string(wxGetCwd().ToUTF8()), space::platform::homeDir(),
                               space::platform::documentsDir());
    apps.scan();

    // Open (or create) the local SQLite database next to the resources directory.
    m_databasePath = resourcesDir.GetPath().ToStdString() + "/spacify.db";
    m_database = std::make_unique<Database>(m_databasePath);
    m_downloadManager = std::make_unique<DownloadManager>(m_database->handle());

    this->m_space = new space::Space();
    this->m_frame = new SpaceFrame(this, this->m_space);
    space::platform::wxwidgets::WxWidgetsGraphicsContext *gc = new space::platform::wxwidgets::WxWidgetsGraphicsContext(this->m_frame);
    this->m_space->setGraphicsContext(gc);
    gc->setWidget(this->m_frame->getSpaceWidget());
    space::SPSkin *skin = new space::SPClassicSkin();
    this->m_windowElement = new space::SPWindowElement(this->m_space, skin);
    this->m_windowElement->setColorScheme(SP_COLOR_SCHEME_LIGHT);
    this->m_frame->setWindowElement(this->m_windowElement);
    this->m_frame->setSpace(m_space);
    this->m_windowElement->setSpace(m_space);
    this->m_space->addWindow(this->m_windowElement);
    this->m_main = new space::MainElement(this->m_windowElement);
    this->m_main->setId("main");

    space::SPColor tintColor(100, 127, 255, 127);
    short colorScheme          = SP_COLOR_SCHEME_DARK;
    bool  glossy               = false;
    std::string skinType       = "modern";    // default on first run
    bool  fullWidthAddressBar  = true;
    float aeroHue              = 200.0f;
    float aeroSaturation       = 0.70f;
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
                if (obj.contains("tintColor")) {
                    string hexColor = obj.at("tintColor").get<string>();
                    tintColor = space::SPColor(hexColor);
                }
                if (obj.contains("colorScheme"))
                    colorScheme = static_cast<short>(obj.at("colorScheme").get<std::int64_t>());
                if (obj.contains("glossy"))
                    glossy = obj.at("glossy").get<bool>();
                if (obj.contains("developerMode"))
                    m_developerMode = obj.at("developerMode").get<bool>();
                if (obj.contains("skinType"))
                    skinType = obj.at("skinType").get<string>();
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

    // Swap to the correct skin now that we know which type to use.
    space::SPSkin* activeSkin = nullptr;
    if (skinType == "frutiger-aero") {
        auto* aero = new space::SPFrutigerAeroSkin();
        aero->setHue(aeroHue);
        aero->setSaturation(aeroSaturation);
        aero->setLightness(aeroLightness);
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
    // The HSL sliders tint every skin, including the skin.xml sprites.
    activeSkin->setHue(aeroHue);
    activeSkin->setSaturation(aeroSaturation);
    activeSkin->setLightness(aeroLightness);
    this->m_windowElement->setSkin(activeSkin);

    this->m_windowElement->setColorScheme(colorScheme);
    if (skinType == "classic")
        this->m_windowElement->setTintColor(space::SPColor(tintColor));
    else
        this->m_windowElement->setTintColor(space::SPColor::fromHSL(aeroHue, aeroSaturation, aeroLightness));
    this->m_main->setFullWidthAddressBar(fullWidthAddressBar);
    this->m_windowElement->appendChild(this->m_main);
    this->m_windowElement->pack();
    this->m_frame->applyDeveloperMode(m_developerMode);
    this->m_frame->Show(true);
    this->m_frame->getSpaceWidget()->Show();

    SetTopWindow(this->m_frame);

    // Initialise the position timer (owned by this wxEvtHandler)
    m_positionTimer.SetOwner(this, ID_POSITION_TIMER);

    return true;
}

int SpacifyApp::OnExit() {
    if (space::SPApplicationContext::current() == this)
        space::SPApplicationContext::setCurrent(nullptr);
    m_positionTimer.Stop();
    for (SPMediaService* svc : m_mediaServices)
        delete svc;
    m_mediaServices.clear();
    // Clean up play queue
    for (SPPlayable* p : m_queuePlayables)
        delete p;
    m_queuePlayables.clear();
    m_queueEntries.clear();
    m_currentPlayable = nullptr;
    return wxApp::OnExit();
}

std::string SpacifyApp::currentPlayableUri() const {
    return m_currentPlayable ? m_currentPlayable->uri : std::string();
}

// ── Service lookup ────────────────────────────────────────────────────────────

SPMediaService* SpacifyApp::getMediaServiceForUri(const std::string& uri) const {
    for (SPMediaService* svc : m_mediaServices) {
        if (svc && svc->acceptsUri(uri))
            return svc;
    }
    return nullptr;
}

// ── Navigation ────────────────────────────────────────────────────────────────

void SpacifyApp::navigate(const std::string& uri) {
    if (m_main) m_main->navigate(uri);
}

void SpacifyApp::showError(const std::string& msg) {
    if (m_main) m_main->showMessage(space::Error, msg);
}

// ── Helpers ───────────────────────────────────────────────────────────────────

/// Extract the filename stem from a URI/file-path, e.g. "/a/b/track.mp3" → "track".
static std::string uriBasename(const std::string& uri) {
    const auto slash = uri.find_last_of("/\\");
    std::string name = (slash == std::string::npos) ? uri : uri.substr(slash + 1);
    const auto dot = name.rfind('.');
    if (dot != std::string::npos) name = name.substr(0, dot);
    return name;
}

// ── Playback ──────────────────────────────────────────────────────────────────

void SpacifyApp::stopCurrentPlayback() {
    m_positionTimer.Stop();
    if (m_currentPlayable && m_currentPlayable->service)
        m_currentPlayable->service->stop();
    m_currentPlayable = nullptr;
    m_isPlaying       = false;
    m_positionMs      = 0;
}

void SpacifyApp::playContext(PlayContext ctx) {
    stopCurrentPlayback();

    // Release old runtime objects
    for (SPPlayable* p : m_queuePlayables)
        delete p;
    m_queuePlayables.clear();
    m_queueEntries.clear();
    m_queueIndex = -1;

    // Build runtime playables for each entry that has a URI
    for (auto& entry : ctx.entries) {
        SPPlayable* p = nullptr;
        if (entry.playable && !entry.uri.empty()) {
            p = new SPPlayable(entry.uri);
            p->resolveService(m_mediaServices);
            if (!p->service) {
                // URI couldn't be resolved — mark slot as unplayable
                entry.playable = false;
                delete p;
                p = nullptr;
            }
        }
        m_queuePlayables.push_back(p); // nullptr for unplayable slots
    }
    m_queueEntries = std::move(ctx.entries);

    // Notify the play queue panel that the full list has changed
    m_playbackEvents.dispatchEvent("queue:changed", nullptr, nullptr);

    // Start playing at the requested index (skips forward past unplayable)
    playFromIndex(static_cast<int>(ctx.startIndex));
}

void SpacifyApp::playFromIndex(int idx) {
    const int size = static_cast<int>(m_queueEntries.size());

    // Advance past any unplayable entries
    while (idx < size && !m_queueEntries[idx].playable)
        ++idx;

    if (idx >= size) {
        // Nothing playable from here — stay stopped
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
    m_positionTimer.Start(500);
}

void SpacifyApp::playQueueAtIndex(int idx) {
    if (idx < 0 || idx >= static_cast<int>(m_queueEntries.size())) return;
    if (!m_queueEntries[idx].playable) return;

    stopCurrentPlayback();
    playFromIndex(idx);
}

// ── Compatibility wrappers (single-entry PlayContext) ─────────────────────────

void SpacifyApp::playTrackId(int64_t trackId) {
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

void SpacifyApp::playUri(const std::string& uri) {
    PlayQueueEntry e;
    e.displayTitle = uriBasename(uri);
    e.uri          = uri;
    e.playable     = true;

    PlayContext ctx;
    ctx.entries.push_back(std::move(e));
    ctx.startIndex = 0;
    playContext(std::move(ctx));
}

void SpacifyApp::togglePlayPause() {
    if (!m_currentPlayable) return;

    m_isPlaying = !m_isPlaying;
    if (m_currentPlayable->service) {
        if (m_isPlaying)
            m_currentPlayable->service->play();
        else
            m_currentPlayable->service->pause();
    }

    if (m_isPlaying)
        m_positionTimer.Start(500);
    else
        m_positionTimer.Stop();

    dispatchPlaybackChanged();
}

void SpacifyApp::seekTo(int posMs) {
    if (!m_currentPlayable) return;

    m_positionMs = posMs;
    if (m_currentPlayable->service)
        m_currentPlayable->service->seek(posMs);

    dispatchPlaybackChanged();
}

void SpacifyApp::playbackNext() {
    stopCurrentPlayback();

    const int next = m_queueIndex + 1;
    if (next < static_cast<int>(m_queueEntries.size()))
        playFromIndex(next);
    else
        dispatchPlaybackChanged(); // end of queue — stay stopped
}

void SpacifyApp::playbackPrev() {
    if (m_positionMs > 3000) {
        seekTo(0);
        return;
    }

    if (m_queueIndex < 0) return; // nothing in queue

    stopCurrentPlayback();

    int prev = m_queueIndex - 1;
    while (prev >= 0 && !m_queueEntries[prev].playable)
        --prev;

    if (prev >= 0)
        playFromIndex(prev);
    else
        playFromIndex(m_queueIndex); // already at first playable — restart
}

// ── Internal helpers ──────────────────────────────────────────────────────────

void SpacifyApp::dispatchPlaybackChanged() {
    auto* args = new space::SPPlaybackStateEventArgs(
        m_isPlaying, m_positionMs, m_durationMs);
    m_playbackEvents.dispatchEvent("playback:changed", args, nullptr);
}

void SpacifyApp::onPositionTimer(wxTimerEvent& /*event*/) {
    if (!m_isPlaying || !m_currentPlayable) return;

    SPMediaService* svc = m_currentPlayable->service;

    if (svc) {
        m_positionMs = svc->getPositionMs();
        // Refresh duration (AVAudioPlayer might not have it ready immediately)
        if (m_durationMs == 0)
            m_durationMs = svc->getDurationMs();
    } else {
        m_positionMs += 500;
    }

    // Detect natural track completion — advance to next in queue
    if (m_durationMs > 0 && m_positionMs >= m_durationMs) {
        m_positionMs = m_durationMs;
        playbackNext();
        return;
    }

    auto* args = new space::SPPlaybackStateEventArgs(
        m_isPlaying, m_positionMs, m_durationMs);
    m_playbackEvents.dispatchEvent("playback:changed", args, nullptr);
}

void SpacifyApp::setDeveloperMode(bool enabled) {
    m_developerMode = enabled;
    if (m_frame) m_frame->applyDeveloperMode(enabled);
}

void SpacifyApp::setFullWidthAddressBar(bool fullWidth) {
    if (!m_main) return;
    m_main->setFullWidthAddressBar(fullWidth);
    if (m_windowElement) {
        m_windowElement->pack();
        auto* gc = m_windowElement->createGraphics();
        m_windowElement->invalidateRegion({0, 0, gc->getWidth(), gc->getHeight()});
    }
}

void SpacifyApp::showWebInspector() {
    if (auto* sv = space::views::SiteView::getActiveInstance())
        sv->showInspector();
}

void SpacifyApp::reloadWebPage() {
    if (auto* sv = space::views::SiteView::getActiveInstance())
        sv->reload();
}
