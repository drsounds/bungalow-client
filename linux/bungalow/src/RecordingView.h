#pragma once

#include "MusicBrowseView.h"

namespace space {
    namespace views {

        /**
         * RecordingView  —  bungalow:recording:<isrc>
         *
         * One recording's appearances: a row per track, i.e. per release it is
         * on. Edit / Delete act on the recording; right-click a row for the
         * shared track menu.
         */
        class RecordingView : public MusicBrowseView {
        public:
            RecordingView(SPElement* parent, Database* db);

            bool        acceptsUri(const std::string& uri) override;
            void        navigate(const std::string& uri) override;
            std::string getType() override { return "recordingview"; }

        protected:
            void reload() override;
            void onActivate(Page& page, int row) override;
            void onContextMenu(Page& page, int row) override;
            void onToolButton(const std::string& id) override;

        private:
            std::string m_isrc;
        };

    } // namespace views
} // namespace space
