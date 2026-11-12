#pragma once

#include "../../space/src/SPTabBarViewElement.h"
#include "MusicEntityDataSource.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

class Database;
class wxWindow;

namespace space {
    class SPTableElement;
    class SPObject;
    class SPEventArgs;

    namespace views {

        /**
         * MusicBrowseView — shared base for the library's artist, recording,
         * release and track views (both the browse lists and the detail pages).
         *
         * Each tab is a Page: a full-height table backed by a
         * MusicEntityDataSource. The base wires the table events and hands
         * subclasses the page and row: double-click → onActivate, right-click →
         * onContextMenu, tab-bar tool button → onToolButton. Subclasses fill the
         * pages in reload(), which runs on every navigate() and after refresh().
         */
        class MusicBrowseView : public SPTabBarViewElement {
        public:
            MusicBrowseView(SPElement* parent, Database* db);
            ~MusicBrowseView() override;

            void navigate(const std::string& uri) override;
            void setHash(const std::string& hash) override;

        protected:
            struct Page {
                std::string                            id;
                SPTableElement*                        table = nullptr;
                std::unique_ptr<MusicEntityDataSource> source;
            };

            /** Add a tab with its table. Pages are shown in the order added. */
            Page& addPage(const std::string& id, const std::string& title,
                          const std::string& uri, std::vector<std::string> columns);

            /** Point every tab at @p uri + "#<page id>" (for per-entity detail views). */
            void setTabBaseUri(const std::string& uri);

            /** Re-query the database into the pages. */
            virtual void reload() = 0;
            virtual void onActivate(Page& page, int row) {}
            virtual void onContextMenu(Page& page, int row) {}
            virtual void onToolButton(const std::string& id) {}

            /** reload() then repaint every table. */
            void refresh();

            // ── Helpers for subclasses ────────────────────────────────────────

            /** Play the page's rows as a queue, starting at @p row. */
            void playFrom(Page& page, int row);

            /** Optional entries of trackContextMenu(). */
            enum TrackMenuFlags : unsigned {
                kTrackOpenRelease   = 1u << 0,
                kTrackOpenRecording = 1u << 1,
            };

            /**
             * The shared right-click menu for a track row: Play, save/unsave,
             * the optional "open" entries, Edit, track number, Delete. Refreshes
             * the view when the data changed.
             */
            void trackContextMenu(Page& page, int row, unsigned flags);
            /** Navigate the main window to @p uri. */
            void open(const std::string& uri);
            /**
             * Pop up a context menu of (id, label) items at the cursor.
             * An empty label inserts a separator. Returns the chosen id or -1.
             */
            int popupMenu(const std::vector<std::pair<int, std::string>>& items);
            bool confirm(const std::string& question);
            wxWindow* topWindow() const;

            Database* m_db = nullptr;
            std::vector<std::unique_ptr<Page>> m_pages;

        private:
            Page* pageFor(SPObject* sender);
            int onRowDblClicked(SPObject* sender, SPEventArgs* args);
            int onRowRightClicked(SPObject* sender, SPEventArgs* args);
            int onToolButtonClicked(SPObject* sender, SPEventArgs* args);
        };

        /** "bungalow:<kind>:<key>" for detail views; keys are used verbatim. */
        std::string entityUri(const std::string& kind, const std::string& key);

        /** The key of a "bungalow:<kind>:<key>" URI, or "" if it doesn't match. */
        std::string entityKey(const std::string& kind, const std::string& uri);

        /** An identifier for display: generated "local:" keys show as "". */
        std::string displayId(const std::string& key);

    } // namespace views
} // namespace space
