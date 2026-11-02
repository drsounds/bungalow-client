#pragma once

#include "feature/Feature.h"

namespace space {
namespace app {

/// Start page and Now Playing (bungalow:start, bungalow:player).
class StartFeature : public feature::Feature {
public:
    std::string getId() const override { return "start"; }
    std::string getName() const override { return "Start"; }
    std::vector<SPViewElement*> createViews(SPViewStackElement* viewStack) override;
    std::vector<navigation::SidebarNode> getSidebarNodes() const override;
};

} // namespace app
} // namespace space
