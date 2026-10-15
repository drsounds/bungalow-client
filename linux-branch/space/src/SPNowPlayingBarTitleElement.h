#pragma once

#include "SPVBoxElement.h"
#include <string>

namespace space {

class SPTextElement;
class SPObject;
class SPEventArgs;
class SPImage;
class SPGraphicsContext;

/// Sidebar widget that shows the currently playing track.
/// Layout (top → bottom): title label, artist/info label, cover art image.
/// Subscribes to SpacifyApp's "playback:changed" observable.
class SPNowPlayingBarTitleElement : public SPVBoxElement
{
public:
    explicit SPNowPlayingBarTitleElement(SPElement* parent);
    ~SPNowPlayingBarTitleElement() override;

    string getType() override { return "nowplayingbartitle"; }

    void paint(int x, int y, SPGraphicsContext* gc) override;

    /// Called when SpacifyApp dispatches a "playback:changed" event.
    int onPlaybackChanged(SPObject* sender, SPEventArgs* args);

private:
    SPTextElement* m_titleLabel = nullptr;
    SPTextElement* m_infoLabel  = nullptr;
};

} // namespace space
