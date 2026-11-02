#include "app/start/StartFeature.h"

#include "PlayerView.h"
#include "StartView.h"

namespace space {
namespace app {

std::vector<SPViewElement*> StartFeature::createViews(SPViewStackElement* viewStack) {
    return {new views::PlayerView(viewStack), new views::StartView(viewStack)};
}

std::vector<navigation::SidebarNode> StartFeature::getSidebarNodes() const {
    navigation::SidebarNode main("MAIN");
    main.addChild(navigation::SidebarNode("Start", "bungalow:start").setIconPath("icons/NowPlaying.svg"));
    main.addChild(navigation::SidebarNode("Now Playing", "bungalow:player").setIconPath("icons/NowPlaying.svg"));
    return {main};
}

} // namespace app
} // namespace space
