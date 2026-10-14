#pragma once

#include "SPVBoxElement.h"
#include <string>

namespace space {

class SPTextElement;
class SPObject;
class SPEventArgs;
class SPImage;
class SPGraphicsContext;
class SPNowPlayingBarHeaderElement;

namespace views {
    class SPPlayerCanvasElement;
}

/// Sidebar widget that shows the currently playing track.
/// Layout (top → bottom): title label, artist/info label, cover art image.
/// Subscribes to SpacifyApp's "playback:changed" observable.
class SPNowPlayingBarElement : public SPVBoxElement
{
public:
    explicit SPNowPlayingBarElement(SPElement* parent);
    ~SPNowPlayingBarElement() override;

    string getType() override { return "nowplayingbar"; }

    void pack() override;

    void paint(int x, int y, SPGraphicsContext* gc) override;

    /// Called when SpacifyApp dispatches a "playback:changed" event.
    int onPlaybackChanged(SPObject* sender, SPEventArgs* args);

private:
    std::string m_coverArtPath;
    SPImage* m_cachedImage = nullptr;
    views::SPPlayerCanvasElement* m_playerCanvas = nullptr;
    SPNowPlayingBarHeaderElement *m_header = nullptr;
};

} // namespace space
