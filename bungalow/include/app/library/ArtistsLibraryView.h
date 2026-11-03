#pragma once

#include "app/library/MusicBrowseView.h"

namespace space {
    namespace views {

        /**
         * ArtistsLibraryView  —  bungalow:library:artists
         *
         * Every artist in the library with how many recordings and releases
         * credit them. + adds an artist; double-click opens the artist page;
         * right-click offers Open / Edit / Delete.
         */
        class ArtistsLibraryView : public MusicBrowseView {
        public:
            ArtistsLibraryView(SPElement* parent, Database* db);

            bool        acceptsUri(const std::string& uri) override;
            std::string getType() override { return "artistslibraryview"; }

        protected:
            void reload() override;
            void onActivate(Page& page, int row) override;
            void onContextMenu(Page& page, int row) override;
            void onToolButton(const std::string& id) override;
        };

    } // namespace views
} // namespace space
