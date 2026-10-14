#include "SPNowPlayingBarHeaderElement.h"
    
#include "SPHBoxElement.h"
#include "SPTextElement.h"
#include "SPGraphicsContext.h"
#include "SPImage.h"
#include "SPPlaybackEventArgs.h"
#include "SPApplicationContext.h"
#include "SPNowPlayingBarTitleElement.h"
#include "SPImageElement.h"

namespace space {

// ── Construction ──────────────────────────────────────────────────────────────

SPNowPlayingBarHeaderElement::SPNowPlayingBarHeaderElement(SPElement* parent)
    : SPHBoxElement(parent)
{
    this->setTagName("nowplayingbarheader");
    this->setGap(4);
    this->setHeight(64);

    m_coverImage = new SPImageElement(this);
    m_coverImage->setWidth(64);
    m_coverImage->setHeight(64);
    this->appendChild(m_coverImage);

    // ── Track title label ──────────────────────────────────────────────────
    m_title = new SPNowPlayingBarTitleElement(this);
    m_title->setFlex(1);
    m_title->setPadding({ 0, 0, 0, 0 });
    this->appendChild(m_title);

    // ── Subscribe to playback events ───────────────────────────────────────
    if (auto* app = SPApplicationContext::current()) {
        app->getPlaybackEvents().addEventListener(
            "playback:changed",
            reinterpret_cast<SPCallback>(&SPNowPlayingBarHeaderElement::onPlaybackChanged),
            (SPElement *)this,
            nullptr);
    }
}

SPNowPlayingBarHeaderElement::~SPNowPlayingBarHeaderElement() {
    delete m_coverImage;
    delete m_title;
}

// ── Playback event handler ────────────────────────────────────────────────────

int SPNowPlayingBarHeaderElement::onPlaybackChanged(SPObject* /*sender*/, SPEventArgs* args) {
    // this->invalidate();
    auto* pba = dynamic_cast<SPPlaybackStateEventArgs*>(args);
    if (!pba) return 0;

    auto* app = SPApplicationContext::current();
    if (!app) return 0;

    /*
    if (m_coverImage) {
        if (app->getCurrentPlayable() && !app->getCurrentPlayable()->getCoverArtPath().empty()) {
            m_coverImage->setSrc(app->getCurrentPlayable()->getCoverArtPath());
        } else {
            m_coverImage->setSrc("resources/default-cover.png");
        }
    }*/

    return 0;
}

// ── Rendering ─────────────────────────────────────────────────────────────────

void SPNowPlayingBarHeaderElement::paint(int x, int y, SPGraphicsContext* gc) {
    // Draw background and text children via the parent implementation.
    SPHBoxElement::paint(x, y, gc);
}

} // namespace space
