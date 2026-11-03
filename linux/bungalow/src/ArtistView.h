#pragma once

#include "MusicBrowseView.h"

namespace space {
    namespace views {

        /**
         * ArtistView  —  bungalow:artist:<isni>[#tracks|#releases]
         *
         * One artist's page: the tracks of every recording they're credited
         * on, and the releases they appear on. Tool buttons edit or delete the
         * artist; rows play / open like the library lists.
         */
        class ArtistView : public MusicBrowseView {
        public:
            ArtistView(SPElement* parent, Database* db);

            bool        acceptsUri(const std::string& uri) override;
            void        navigate(const std::string& uri) override;
            std::string getType() override { return "artistview"; }

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
