#pragma once

#include "app/library/MusicBrowseView.h"

namespace space {
    namespace views {

        /**
         * ReleaseDetailView  —  bungalow:library:release:<upc>
         *
         * One release's tracklist in release order. + adds a recording as the
         * next track; Edit / Delete act on the release; right-click a track
         * for the shared track menu (renumber, delete, save…).
         */
        class ReleaseDetailView : public MusicBrowseView {
        public:
            ReleaseDetailView(SPElement* parent, Database* db);

            bool        acceptsUri(const std::string& uri) override;
            void        navigate(const std::string& uri) override;
            std::string getType() override { return "releasedetailview"; }

        protected:
            void reload() override;
            void onActivate(Page& page, int row) override;
            void onContextMenu(Page& page, int row) override;
            void onToolButton(const std::string& id) override;

        private:
            std::string m_upc;
        };

    } // namespace views
} // namespace space
