#pragma once

#include "MusicBrowseView.h"

namespace space {
    namespace views {

        /**
         * ReleasesView  —  bungalow:library:releases
         *
         * Every release with its artists, date, track count and UPC.
         * + adds a release (then opens it to add tracks); double-click opens
         * the release page; right-click offers Open / Edit / Delete.
         */
        class ReleasesView : public MusicBrowseView {
        public:
            ReleasesView(SPElement* parent, Database* db);

            bool        acceptsUri(const std::string& uri) override;
            std::string getType() override { return "releasesview"; }

        protected:
            void reload() override;
            void onActivate(Page& page, int row) override;
            void onContextMenu(Page& page, int row) override;
            void onToolButton(const std::string& id) override;
        };

    } // namespace views
} // namespace space
