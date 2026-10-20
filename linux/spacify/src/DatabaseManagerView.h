#pragma once

#include "../../space/src/SPTabBarViewElement.h"
#include "../../space/src/SPTableDataSource.h"

#include <sqlite3.h>
#include <string>
#include <vector>

#include "Database.h"

namespace space {
    class SPTableElement;
    class SPObject;
    class SPEventArgs;

    // ── Inline data source for the list of custom tables ─────────────────────

    class DbTableListDataSource : public SPTableDataSource {
    public:
        explicit DbTableListDataSource(sqlite3* db);
        void reload(sqlite3* db);

        int         getColumnCount()                      override { return 2; }
        int         getRowCount()                         override;
        std::string getTextForColumn(int column)          override;
        std::string getTextForCell  (int column, int row) override;
        bool        isRowPlayable   (int /*row*/)         override { return false; }

        /** Returns the table name for the given row index, or "" if out of range. */
        std::string getTableName(int row) const;

    private:
        std::vector<CustomTableDef> m_tables;
        void load(sqlite3* db);
    };

    namespace views {

        /**
         * DatabaseManagerView  —  bungalow:database
         *
         * Shows the list of user-defined tables with:
         *   + New Table button  →  CreateTableDialog
         *   ✕ Delete button     →  drop the selected table
         *   Double-click row    →  navigate to bungalow:table:<name>
         */
        class DatabaseManagerView : public SPTabBarViewElement {
        public:
            DatabaseManagerView();
            explicit DatabaseManagerView(SPElement* parent, sqlite3* db = nullptr);
            ~DatabaseManagerView() override;

            bool        acceptsUri(const std::string& uri) override;
            void        navigate  (const std::string& uri) override;
            void        setHash   (const std::string& hash) override;
            std::string getType()                           override { return "databasemanagerview"; }

        private:
            int onRowDblClicked    (SPObject* sender, SPEventArgs* args);
            int onToolButtonClicked(SPObject* sender, SPEventArgs* args);

            SPTableElement*         m_table      = nullptr;
            DbTableListDataSource*  m_dataSource = nullptr;
            sqlite3*                m_db         = nullptr;
        };

    } // namespace views
} // namespace space
