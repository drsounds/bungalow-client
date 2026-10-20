#include "SPNowPlayingBarElement.h"

#include "SPTextElement.h"
#include "SPGraphicsContext.h"
#include "SPImage.h"
#include "SPPlaybackEventArgs.h"
#include "../../bungalow/src/BungalowApp.h"
#include "SPNowPlayingBarHeaderElement.h"
#include "SPPlayerCanvasElement.h"

#include <wx/app.h>

namespace space {

// ── Construction ──────────────────────────────────────────────────────────────

SPNowPlayingBarElement::SPNowPlayingBarElement(SPElement* parent)
    : SPVBoxElement(parent)
{
    this->setTagName("nowplayingbar");
    this->setPadding({ 0, 0, 0, 0 });
    this->setGap(4);
    this->setHeight(128 + 64);
    m_header = new SPNowPlayingBarHeaderElement(this);
this->appendChild(m_header);

    m_playerCanvas = new views::SPPlayerCanvasElement(this);
    m_playerCanvas->setHeight(128); // hidden until we have cover art to show
    m_playerCanvas->setFlex(1);

    this->appendChild(m_playerCanvas);

    // Rebuild the parameter panel whenever the active implementation changes.
    m_playerCanvas->addEventListener(
        "implementationChanged",
        reinterpret_cast<SPCallback>(&SPNowPlayingBarElement::onPlaybackChanged),
        this,
        nullptr
    );

    // ── Subscribe to playback events ───────────────────────────────────────
    if (auto* app = dynamic_cast<BungalowApp*>(wxTheApp)) {
        app->getPlaybackEvents().addEventListener(
            "playback:changed",
            reinterpret_cast<SPCallback>(&SPNowPlayingBarElement::onPlaybackChanged),
            this,
            nullptr);
    }
}

SPNowPlayingBarElement::~SPNowPlayingBarElement() {
    delete m_header;
    delete m_playerCanvas;
}

// ── Playback event handler ────────────────────────────────────────────────────

int SPNowPlayingBarElement::onPlaybackChanged(SPObject* /*sender*/, SPEventArgs* args) {
    auto* pba = dynamic_cast<SPPlaybackStateEventArgs*>(args);
    if (!pba) return 0;

    auto* app = dynamic_cast<BungalowApp*>(wxTheApp);
    if (!app) return 0;

    // m_playerCanvas->setPlaybackState(pba->isPlaying(), pba->getPositionMs(), pba->getDurationMs());

    this->invalidate();
    return 0;
}

void SPNowPlayingBarElement::pack() {
    SPVBoxElement::pack();
    if (m_header) m_header->pack();
    this->setHeight(this->getWidth() + 100);
}

// ── Rendering ─────────────────────────────────────────────────────────────────

void SPNowPlayingBarElement::paint(int x, int y, SPGraphicsContext* gc) {
    // Draw background and text children via the parent implementation.
    SPVBoxElement::paint(x, y, gc);
}

} // namespace space
