#pragma once

#include "app/library/MusicBrowseView.h"

namespace space {
    namespace views {

        /**
         * RecordingsLibraryView  —  bungalow:library:recordings
         *
         * Every recording with its version, credited artists and ISRC.
         * Double-click plays it (through any playable track of it); + adds a
         * recording; right-click offers Play / Open / Edit / Delete.
         */
        class RecordingsLibraryView : public MusicBrowseView {
        public:
            RecordingsLibraryView(SPElement* parent, Database* db);

            bool        acceptsUri(const std::string& uri) override;
            std::string getType() override { return "recordingslibraryview"; }

        protected:
            void reload() override;
            void onActivate(Page& page, int row) override;
            void onContextMenu(Page& page, int row) override;
            void onToolButton(const std::string& id) override;
        };

    } // namespace views
} // namespace space
