#pragma once

#include "app/library/MusicBrowseView.h"

namespace space {
    namespace views {

        /**
         * ArtistDetailView  —  bungalow:library:artist:<isni>[#tracks|#releases]
         *
         * One artist's page: the tracks of every recording they're credited
         * on, and the releases they appear on. Tool buttons edit or delete the
         * artist; rows play / open like the library lists.
         */
        class ArtistDetailView : public MusicBrowseView {
        public:
            ArtistDetailView(SPElement* parent, Database* db);

            bool        acceptsUri(const std::string& uri) override;
            void        navigate(const std::string& uri) override;
            std::string getType() override { return "artistdetailview"; }

        protected:
            void reload() override;
            void onActivate(Page& page, int row) override;
            void onContextMenu(Page& page, int row) override;
            void onToolButton(const std::string& id) override;

        private:
            std::string m_isni;
        };

    } // namespace views
} // namespace space
