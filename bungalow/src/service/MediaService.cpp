#include "service/MediaService.h"

#include <algorithm>

namespace space {
namespace service {

void MediaService::addPlaybackListener(PlaybackListener* listener) {
    if (listener && std::find(m_listeners.begin(), m_listeners.end(), listener) == m_listeners.end())
        m_listeners.push_back(listener);
}

void MediaService::removePlaybackListener(PlaybackListener* listener) {
    m_listeners.erase(std::remove(m_listeners.begin(), m_listeners.end(), listener), m_listeners.end());
}

// Each fire* iterates a copy, so a listener may add or remove listeners.

void MediaService::setPlaybackState(PlaybackState state) {
    if (state == m_state) return;
    m_state = state;
    for (PlaybackListener* l : std::vector<PlaybackListener*>(m_listeners)) l->onStateChanged(state);
}

void MediaService::firePositionChanged(int positionMs, int durationMs) {
    for (PlaybackListener* l : std::vector<PlaybackListener*>(m_listeners))
        l->onPositionChanged(positionMs, durationMs);
}

void MediaService::fireTrackChanged(const std::string& title, const std::string& artist,
                                    const std::string& album) {
    for (PlaybackListener* l : std::vector<PlaybackListener*>(m_listeners))
        l->onTrackChanged(title, artist, album);
}

void MediaService::fireError(const std::string& message) {
    setPlaybackState(PlaybackState::Error);
    for (PlaybackListener* l : std::vector<PlaybackListener*>(m_listeners)) l->onError(message);
}

void MediaService::fireCompleted() {
    for (PlaybackListener* l : std::vector<PlaybackListener*>(m_listeners)) l->onCompleted();
}

} // namespace service
} // namespace space
