#pragma once

#include "MusicBrowseView.h"

namespace space {
    namespace views {

        /**
         * TracksView  —  bungalow:library:tracks
         *
         * Every track (a recording's appearance on a release), whether or not
         * it is saved to the library — the saved subset is LibraryView.
         * Double-click plays from that row; + opens AddTrackDialog; right-click
         * gives the shared track menu.
         */
        class TracksView : public MusicBrowseView {
        public:
            TracksView(SPElement* parent, Database* db);

            bool        acceptsUri(const std::string& uri) override;
            std::string getType() override { return "tracksview"; }

        protected:
            void reload() override;
            void onActivate(Page& page, int row) override;
            void onContextMenu(Page& page, int row) override;
            void onToolButton(const std::string& id) override;
        };

    } // namespace views
} // namespace space
