#include "app/accounts/AccountsFeature.h"

#include "app/AppContext.h"
#include "app/accounts/AccountsView.h"

namespace space {
namespace app {

std::vector<SPViewElement*> AccountsFeature::createViews(SPViewStackElement* viewStack) {
    return {new AccountsView(viewStack, serviceManager())};
}

std::vector<navigation::SidebarNode> AccountsFeature::getSidebarNodes() const {
    navigation::SidebarNode library("LIBRARY");
    library.addChild(navigation::SidebarNode("Accounts", "bungalow:accounts").setIconPath("icons/Document.svg"));
    return {library};
}

} // namespace app
} // namespace space
