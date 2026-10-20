#pragma once

#include "SPHBoxElement.h"
#include <string>

namespace space {

class SPTextElement;
class SPObject;
class SPEventArgs;
class SPImage;
class SPImageElement;
class SPGraphicsContext;
class SPNowPlayingBarTitleElement;

/// Sidebar widget that shows the currently playing track.
/// Layout (left → right): title label, artist/info label, cover art image.
/// Subscribes to BungalowApp's "playback:changed" observable.
class SPNowPlayingBarHeaderElement : public SPHBoxElement
{
public:
    explicit SPNowPlayingBarHeaderElement(SPElement* parent);
    ~SPNowPlayingBarHeaderElement() override;

    string getType() override { return "nowplayingbarheader"; }

    void paint(int x, int y, SPGraphicsContext* gc) override;

    /// Called when BungalowApp dispatches a "playback:changed" event.
    int onPlaybackChanged(SPObject* sender, SPEventArgs* args);

private:
    SPImageElement* m_coverImage = nullptr;
    SPNowPlayingBarTitleElement* m_title = nullptr;
};

} // namespace space
