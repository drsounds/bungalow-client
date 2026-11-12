#pragma once

#include "MusicBrowseView.h"

namespace space {
    namespace views {

        /**
         * ArtistsView  —  bungalow:library:artists
         *
         * Every artist in the library with how many recordings and releases
         * credit them. + adds an artist; double-click opens the artist page;
         * right-click offers Open / Edit / Delete.
         */
        class ArtistsView : public MusicBrowseView {
        public:
            ArtistsView(SPElement* parent, Database* db);

            bool        acceptsUri(const std::string& uri) override;
            std::string getType() override { return "artistsview"; }

        protected:
            void reload() override;
            void onActivate(Page& page, int row) override;
            void onContextMenu(Page& page, int row) override;
            void onToolButton(const std::string& id) override;
        };

    } // namespace views
} // namespace space
