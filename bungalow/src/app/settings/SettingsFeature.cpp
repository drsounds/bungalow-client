#include "app/settings/SettingsFeature.h"

#include "ConfigView.h"

namespace space {
namespace app {

std::vector<SPViewElement*> SettingsFeature::createViews(SPViewStackElement* viewStack) {
    return {new views::ConfigView(viewStack)};
}

std::vector<navigation::SidebarNode> SettingsFeature::getSidebarNodes() const {
    navigation::SidebarNode library("LIBRARY");
    library.addChild(navigation::SidebarNode("Settings", "bungalow:config")
                         .setIconPath("icons/Document.svg")
                         .setExpanded(true));
    return {library};
}

} // namespace app
} // namespace space
