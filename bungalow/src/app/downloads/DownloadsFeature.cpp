#include "app/downloads/DownloadsFeature.h"

#include "app/AppContext.h"

#include "DownloadsView.h"

namespace space {
namespace app {

std::vector<SPViewElement*> DownloadsFeature::createViews(SPViewStackElement* viewStack) {
    return {new views::DownloadsView(viewStack, downloadManager())};
}

std::vector<navigation::SidebarNode> DownloadsFeature::getSidebarNodes() const {
    navigation::SidebarNode library("LIBRARY");
    library.addChild(navigation::SidebarNode("Downloads", "bungalow:downloads").setIconPath("icons/Downloads.svg"));
    return {library};
}

} // namespace app
} // namespace space
