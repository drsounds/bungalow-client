#include "app/library/LibraryConcept.h"

#include "app/AppContext.h"
#include "app/library/ArtistDetailView.h"
#include "app/library/ArtistsLibraryView.h"
#include "app/library/LibrarySearchProvider.h"
#include "app/library/RecordingDetailView.h"
#include "app/library/RecordingsLibraryView.h"
#include "app/library/ReleaseDetailView.h"
#include "app/library/ReleasesLibraryView.h"
#include "app/library/TracksLibraryView.h"
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
        Database* library = database();
        return {
            new views::LibraryView(viewStack, db),
            new views::LocalFilesView(viewStack, db),
            new views::ArtistsLibraryView(viewStack, library),
            new views::ReleasesLibraryView(viewStack, library),
            new views::RecordingsLibraryView(viewStack, library),
            new views::TracksLibraryView(viewStack, library),
            new views::ArtistDetailView(viewStack, library),
            new views::ReleaseDetailView(viewStack, library),
            new views::RecordingDetailView(viewStack, library),
        };
    }

    std::vector<navigation::SidebarNode> getSidebarNodes() const override {
        navigation::SidebarNode libraryItem("Library", "bungalow:library");
        libraryItem.setIconPath("icons/Library.svg");
        libraryItem.addChild(navigation::SidebarNode("Tracks", "bungalow:library").setIconPath("icons/MusicNote.svg"));
        // Browse the whole music database, not just saved tracks.
        libraryItem.addChild(navigation::SidebarNode("Artists", "bungalow:library:artists").setIconPath("icons/MusicNote.svg"));
        libraryItem.addChild(navigation::SidebarNode("Releases", "bungalow:library:releases").setIconPath("icons/MusicNote.svg"));
        libraryItem.addChild(navigation::SidebarNode("Recordings", "bungalow:library:recordings").setIconPath("icons/MusicNote.svg"));
        libraryItem.addChild(navigation::SidebarNode("All Tracks", "bungalow:library:tracks").setIconPath("icons/MusicNote.svg"));
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
