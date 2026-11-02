#include "app/library/LibraryConcept.h"

#include "app/AppContext.h"
#include "app/library/LibrarySearchProvider.h"
#include "concept/ConceptContext.h"
#include "feature/Feature.h"

#include "LibraryView.h"
#include "LocalFilesView.h"

namespace space {
namespace app {
namespace {

class LibraryFeature : public feature::Feature {
public:
    std::string getId() const override { return "library"; }
    std::string getName() const override { return "Library"; }

    std::vector<SPViewElement*> createViews(SPViewStackElement* viewStack) override {
        sqlite3* db = databaseHandle();
        return {new views::LibraryView(viewStack, db), new views::LocalFilesView(viewStack, db)};
    }

    std::vector<navigation::SidebarNode> getSidebarNodes() const override {
        navigation::SidebarNode libraryItem("Library", "bungalow:library");
        libraryItem.setIconPath("icons/Library.svg");
        libraryItem.addChild(navigation::SidebarNode("Tracks", "bungalow:library").setIconPath("icons/MusicNote.svg"));
        libraryItem.addChild(navigation::SidebarNode("Local Files", "bungalow:localfiles").setIconPath("icons/MusicFile.svg"));

        navigation::SidebarNode section("LIBRARY");
        section.addChild(libraryItem);
        return {section};
    }
};

} // namespace

void LibraryConcept::onActivate(concepts::ConceptContext& ctx) {
    ctx.registerFeature(std::make_unique<LibraryFeature>());
    ctx.registerSearchProvider(std::make_unique<LibrarySearchProvider>());
}

} // namespace app
} // namespace space
