#pragma once

#include "../../space/src/SPTabBarViewElement.h"

#include <sqlite3.h>
#include <string>

namespace space {
    class SPTableElement;
    class LocalFilesDataSource;
    class SPObject;
    class SPEventArgs;

    namespace views {

        /**
         * LocalFilesView  —  spacify:localfiles
         *
         * Displays every row from audio_files in a five-column table:
         *   Name | Version | Artists | Album | Duration
         */
        class LocalFilesView : public SPTabBarViewElement {
        public:
            LocalFilesView();
            explicit LocalFilesView(SPElement* parent, sqlite3* db = nullptr);
            ~LocalFilesView() override;

            bool        acceptsUri(const std::string& uri) override;
            void        navigate(const std::string& uri)  override;
            void        setHash(const std::string& hash)  override;
            std::string getType() override { return "localfilesview"; }

        private:
            int onTableDoubleClick(SPObject* sender, SPEventArgs* args);

            SPTableElement* m_table = nullptr;
            LocalFilesDataSource* m_dataSource = nullptr;
        };

    } // namespace views
} // namespace space
