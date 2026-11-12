#include "LocalMediaService.h"

// ── Stub ──────────────────────────────────────────────────────────────────────
// TODO: implement using Windows Media Foundation or DirectShow

bool LocalMediaService::acceptsUri(const std::string& uri) const {
    return uri.rfind("file://", 0) == 0;
}

void LocalMediaService::load(const std::string& uri) {
    m_loadedUri = uri;
    m_playing   = false;
}

void LocalMediaService::play()          { m_playing = true; }
void LocalMediaService::pause()         { m_playing = false; }
void LocalMediaService::stop()          { m_playing = false; }
void LocalMediaService::seek(int)       {}
int  LocalMediaService::getDurationMs() const { return 0; }
int  LocalMediaService::getPositionMs() const { return 0; }
