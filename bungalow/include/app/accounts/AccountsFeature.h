#pragma once

#include "feature/Feature.h"

namespace space {
namespace app {

/// The signed-in accounts of the services that have a sign-in (bungalow:accounts).
class AccountsFeature : public feature::Feature {
public:
    std::string getId() const override { return "accounts"; }
    std::string getName() const override { return "Accounts"; }
    std::vector<SPViewElement*> createViews(SPViewStackElement* viewStack) override;
    std::vector<navigation::SidebarNode> getSidebarNodes() const override;
};

} // namespace app
} // namespace space
