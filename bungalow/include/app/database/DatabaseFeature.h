#pragma once

#include "feature/Feature.h"

namespace space {
namespace app {

/// The table manager and one view per user-defined table (bungalow:database, bungalow:table:<name>).
class DatabaseFeature : public feature::Feature {
public:
    std::string getId() const override { return "database"; }
    std::string getName() const override { return "Database"; }
    std::vector<SPViewElement*> createViews(SPViewStackElement* viewStack) override;
    std::vector<navigation::SidebarNode> getSidebarNodes() const override;
};

} // namespace app
} // namespace space
