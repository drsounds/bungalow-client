#include "SPAppFooterElement.h"

#include "SPElement.h"
#include "SPHBoxElement.h"
#include "SPButtonElement.h"
#include "SPTextElement.h"
#include "SPSeekBarElement.h"
#include "SPPlaybackEventArgs.h"

#include <cstdio>
#include "SPApplicationContext.h"

namespace space {

// ── Construction ──────────────────────────────────────────────────────────────

SPAppFooterElement::SPAppFooterElement(SPElement *parent) : SPHBoxElement(parent)
{
    this->setTagName("footer");
    this->setColorAttribute("bgcolor", SPColor(57, 57, 57, 255));
    this->setPadding({ 4, 8, 4, 8 });
    this->setGap(6);

    // ── Previous button ────────────────────────────────────────────────────
    m_prevBtn = new SPButtonElement(this);
    m_prevBtn->setInnerText("|<");
    m_prevBtn->setWidth(28);
    m_prevBtn->setHeight(22);
    m_prevBtn->addEventListener(
        "click",
        reinterpret_cast<SPCallback>(&SPAppFooterElement::onPrev),
        this, m_prevBtn);
    this->appendChild(m_prevBtn);

    // ── Play/Pause button ──────────────────────────────────────────────────
    m_playPauseBtn = new SPButtonElement(this);
    m_playPauseBtn->setInnerText(">");
    m_playPauseBtn->setWidth(28);
    m_playPauseBtn->setHeight(22);
    m_playPauseBtn->addEventListener(
        "click",
        reinterpret_cast<SPCallback>(&SPAppFooterElement::onPlayPause),
        this, m_playPauseBtn);
    this->appendChild(m_playPauseBtn);

    // ── Next button ────────────────────────────────────────────────────────
    m_nextBtn = new SPButtonElement(this);
    m_nextBtn->setInnerText(">|");
    m_nextBtn->setWidth(28);
    m_nextBtn->setHeight(22);
    m_nextBtn->addEventListener(
        "click",
        reinterpret_cast<SPCallback>(&SPAppFooterElement::onNext),
        this, m_nextBtn);
    this->appendChild(m_nextBtn);

    // ── Position label ─────────────────────────────────────────────────────
    m_posLabel = new SPTextElement(this);
    m_posLabel->setInnerText("0:00");
    m_posLabel->setWidth(38);
    m_posLabel->setHeight(22);
    m_posLabel->setForegroundColor(SPColor(200, 200, 200, 255));
    this->appendChild(m_posLabel);

    // ── Seek bar ───────────────────────────────────────────────────────────
    m_seekBar = new SPSeekBarElement(this);
    m_seekBar->setFlex(1);
    m_seekBar->setHeight(22);
    m_seekBar->addEventListener(
        "seek",
        reinterpret_cast<SPCallback>(&SPAppFooterElement::onSeek),
        this, m_seekBar);
    this->appendChild(m_seekBar);

    // ── Duration label ─────────────────────────────────────────────────────
    m_durLabel = new SPTextElement(this);
    m_durLabel->setInnerText("0:00");
    m_durLabel->setWidth(38);
    m_durLabel->setHeight(22);
    m_durLabel->setForegroundColor(SPColor(200, 200, 200, 255));
    this->appendChild(m_durLabel);

    // ── Subscribe to playback events from BungalowApp ───────────────────────
    if (auto* app = SPApplicationContext::current()) {
        app->getPlaybackEvents().addEventListener(
            "playback:changed",
            reinterpret_cast<SPCallback>(&SPAppFooterElement::onPlaybackChanged),
            this,
            nullptr);
    }
}

SPAppFooterElement::~SPAppFooterElement() {}

// ── Playback event handler ────────────────────────────────────────────────────

int SPAppFooterElement::onPlaybackChanged(SPObject* /*sender*/, SPEventArgs* args) {
    auto* pba = dynamic_cast<SPPlaybackStateEventArgs*>(args);
    if (!pba) return 0;

    // Update play/pause button label
    if (m_playPauseBtn)
        m_playPauseBtn->setInnerText(pba->playing ? "||" : ">");

    /*
    // Update track title
    if (m_trackLabel) {
        if (auto* app = SPApplicationContext::current())
            m_trackLabel->setInnerText(app->getTrackTitle());
    }
    */

    // Update position label
    if (m_posLabel)
        m_posLabel->setInnerText(formatTime(pba->positionMs));

    // Update duration label
    if (m_durLabel)
        m_durLabel->setInnerText(formatTime(pba->durationMs));

    // Update seek bar
    if (m_seekBar) {
        m_seekBar->setDuration(pba->durationMs);
        m_seekBar->setPosition(pba->positionMs);
    }

    this->invalidate();
    return 0;
}

// ── Seek bar event handler ────────────────────────────────────────────────────

int SPAppFooterElement::onSeek(SPObject* /*sender*/, SPEventArgs* args) {
    auto* sa = dynamic_cast<SPSeekEventArgs*>(args);
    if (!sa) return 0;

    if (auto* app = SPApplicationContext::current())
        app->seekTo(sa->getPositionMs());

    return 0;
}

// ── Button handlers ───────────────────────────────────────────────────────────

int SPAppFooterElement::onPlayPause(SPObject* /*sender*/, SPEventArgs* /*args*/) {
    if (auto* app = SPApplicationContext::current())
        app->togglePlayPause();
    return 0;
}

int SPAppFooterElement::onPrev(SPObject* /*sender*/, SPEventArgs* /*args*/) {
    if (auto* app = SPApplicationContext::current())
        app->playbackPrev();
    return 0;
}

int SPAppFooterElement::onNext(SPObject* /*sender*/, SPEventArgs* /*args*/) {
    if (auto* app = SPApplicationContext::current())
        app->playbackNext();
    return 0;
}

// ── Helpers ───────────────────────────────────────────────────────────────────

std::string SPAppFooterElement::formatTime(int ms) {
    if (ms < 0) ms = 0;
    const int totalSec = ms / 1000;
    const int minutes  = totalSec / 60;
    const int seconds  = totalSec % 60;
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d:%02d", minutes, seconds);
    return buf;
}

} // namespace space
