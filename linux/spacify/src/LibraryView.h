#pragma once

#include "../../space/src/SPTabBarViewElement.h"

#include <sqlite3.h>
#include <string>

namespace space {
    class SPTableElement;
    class LibraryDataSource;
    class SPObject;
    class SPEventArgs;

    namespace views {

        /**
         * LibraryView  —  bungalow:library
         *
         * Displays the user's saved tracks in a three-column table:
         *   Name | Artists | Release
         *
         * Double-clicking a row looks up the track's playable URI and starts
         * playback via BungalowApp::playTrackId().
         */
        class LibraryView : public SPTabBarViewElement {
        public:
            LibraryView();
            explicit LibraryView(SPElement* parent, sqlite3* db = nullptr);
            ~LibraryView() override;

            bool        acceptsUri(const std::string& uri) override;
            void        navigate(const std::string& uri)  override;
            void        setHash(const std::string& hash)  override;
            std::string getType()                          override { return "libraryview"; }

        private:
            void playRowInContext(int row);
            int  onRowDblClicked(SPObject* sender, SPEventArgs* args);
            int  onToolButtonClicked(SPObject* sender, SPEventArgs* args);
            int  onRowRightClicked(SPObject* sender, SPEventArgs* args);

            SPTableElement*    m_table      = nullptr;
            LibraryDataSource* m_dataSource = nullptr;
            sqlite3*           m_db         = nullptr;
        };

    } // namespace views
} // namespace space
