#pragma once

#include "../../space/src/SPTabBarViewElement.h"
#include "CustomTableDataSource.h"

#include <sqlite3.h>
#include <string>

namespace space {
    class SPTableElement;
    class SPObject;
    class SPEventArgs;

    namespace views {

        /**
         * CustomTableView  —  bungalow:table:<name>
         *
         * Displays rows of a single user-defined table with full CRUD:
         *   + tool button  →  CustomRowDialog (add mode)
         *   right-click row → Edit (CustomRowDialog edit mode) / Delete
         */
        class CustomTableView : public SPTabBarViewElement {
        public:
            CustomTableView();
            explicit CustomTableView(SPElement* parent, sqlite3* db = nullptr);
            ~CustomTableView() override;

            bool        acceptsUri(const std::string& uri) override;
            void        navigate  (const std::string& uri) override;
            void        setHash   (const std::string& hash) override;
            std::string getType()                           override { return "customtableview"; }

        private:
            int onToolButtonClicked(SPObject* sender, SPEventArgs* args);
            int onRowDblClicked    (SPObject* sender, SPEventArgs* args);
            int onRowRightClicked  (SPObject* sender, SPEventArgs* args);

            SPTableElement*        m_table      = nullptr;
            CustomTableDataSource* m_dataSource = nullptr;
            sqlite3*               m_db         = nullptr;
            std::string            m_tableName;
        };

    } // namespace views
} // namespace space
