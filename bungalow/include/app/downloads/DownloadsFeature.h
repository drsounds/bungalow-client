#pragma once

#include "feature/Feature.h"

namespace space {
namespace app {

/// Ongoing and completed downloads (bungalow:downloads).
class DownloadsFeature : public feature::Feature {
public:
    std::string getId() const override { return "downloads"; }
    std::string getName() const override { return "Downloads"; }
    std::vector<SPViewElement*> createViews(SPViewStackElement* viewStack) override;
    std::vector<navigation::SidebarNode> getSidebarNodes() const override;
};

} // namespace app
} // namespace space
