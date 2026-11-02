#pragma once

#include "feature/Feature.h"

namespace space {
namespace app {

/// Application settings (bungalow:config).
class SettingsFeature : public feature::Feature {
public:
    std::string getId() const override { return "settings"; }
    std::string getName() const override { return "Settings"; }
    std::vector<SPViewElement*> createViews(SPViewStackElement* viewStack) override;
    std::vector<navigation::SidebarNode> getSidebarNodes() const override;
};

} // namespace app
} // namespace space
