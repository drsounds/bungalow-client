#include "SPNowPlayingBarTitleElement.h"

#include "SPVBoxElement.h"
#include "SPTextElement.h"
#include "SPGraphicsContext.h"
#include "SPImage.h"
#include "SPPlaybackEventArgs.h"
#include "../../bungalow/src/BungalowApp.h"

#include <wx/app.h>

namespace space {

// ── Construction ──────────────────────────────────────────────────────────────

SPNowPlayingBarTitleElement::SPNowPlayingBarTitleElement(SPElement* parent)
    : SPVBoxElement(parent)
{
    this->setTagName("nowplayingbartitle");
    this->setPadding({ 8, 8, 8, 8 });
    this->setGap(4);
    this->setHeight(170);
    this->setBackgroundColor(SPColor(0, 0, 0, 0));

    // ── Track title label ──────────────────────────────────────────────────
    m_titleLabel = new SPTextElement(this);
    m_titleLabel->setBackgroundColor(SPColor(0, 0, 0, 0));
    m_titleLabel->setInnerText("");
    m_titleLabel->setHeight(18);
    m_titleLabel->setIntegerAttribute("size", 12);
    this->appendChild(m_titleLabel);

    // ── Artist / info label ────────────────────────────────────────────────
    m_infoLabel = new SPTextElement(this);
    m_infoLabel->setBackgroundColor(SPColor(0, 0, 0, 0));
    m_infoLabel->setInnerText("");
    m_infoLabel->setHeight(14);
    m_infoLabel->setIntegerAttribute("size", 10);
    this->appendChild(m_infoLabel);

    // ── Subscribe to playback events ───────────────────────────────────────
    if (auto* app = dynamic_cast<BungalowApp*>(wxTheApp)) {
        app->getPlaybackEvents().addEventListener(
            "playback:changed",
            reinterpret_cast<SPCallback>(&SPNowPlayingBarTitleElement::onPlaybackChanged),
            this,
            nullptr);
    }
}

SPNowPlayingBarTitleElement::~SPNowPlayingBarTitleElement() {
}

// ── Playback event handler ────────────────────────────────────────────────────

int SPNowPlayingBarTitleElement::onPlaybackChanged(SPObject* /*sender*/, SPEventArgs* args) {
    auto* pba = dynamic_cast<SPPlaybackStateEventArgs*>(args);
    if (!pba) return 0;

    auto* app = dynamic_cast<BungalowApp*>(wxTheApp);
    if (!app) return 0;

    if (app->getCurrentPlayable()) {
        if (m_titleLabel) m_titleLabel->setInnerText(app->getTrackTitle());
        if (m_infoLabel)  m_infoLabel->setInnerText(app->getArtistName());

    } else {
        if (m_titleLabel) m_titleLabel->setInnerText("Nothing playing");
        if (m_infoLabel)  m_infoLabel->setInnerText("");
    }

    this->invalidate();
    return 0;
}

// ── Rendering ─────────────────────────────────────────────────────────────────

void SPNowPlayingBarTitleElement::paint(int x, int y, SPGraphicsContext* gc) {
    // Draw background and text children via the parent implementation.
    SPVBoxElement::paint(x, y, gc);
}

} // namespace space
