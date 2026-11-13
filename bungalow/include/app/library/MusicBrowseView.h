#pragma once

#include "SPTabBarViewElement.h"
#include "app/library/MusicEntityDataSource.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

class Database;

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
         * pages with entities from a MusicEntityRepository in reload(), which
         * runs on every navigate() and after refresh().
         * Rows are dragged out by the table itself; a view that takes drops
         * says so with acceptsDrop() and gets them in onDropRefs().
         * Dialogs and menus go through platform::, so the views work on every
         * UI backend.
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
                          const std::string& uri, std::vector<MusicColumn> columns);

            /** Point every tab at @p uri + "#<page id>" (for per-entity detail views). */
            void setTabBaseUri(const std::string& uri);

            /** Re-query the database into the pages. */
            virtual void reload() = 0;
            virtual void onActivate(Page& page, int row) {}
            virtual void onContextMenu(Page& page, int row) {}
            virtual void onToolButton(const std::string& id) {}
            /** Whether dragged entities can be dropped into the view's tables. */
            virtual bool acceptsDrop() const { return false; }
            /** Entities, as their URIs, were dropped in front of row @p position. */
            virtual bool onDropRefs(const std::vector<std::string>& refs, int position) {
                (void)refs;
                (void)position;
                return false;
            }
            /** Rows of this view's own table were dragged to in front of row @p position (a reorder). */
            virtual bool onMoveRows(const std::vector<int>& rows, int position,
                                    std::vector<int>& newRows) {
                (void)rows;
                (void)position;
                (void)newRows;
                return false;
            }

            /**
             * reload() then repaint every table. Availability is asked again,
             * since what the library holds may have changed.
             */
            void refresh();

            // ── Helpers for subclasses ────────────────────────────────────────

            /**
             * Play the page's rows as a queue, starting at @p row.
             * @p chooseService is "Play with…": ask which service plays the
             * row, which also lets a row without a local file try the others.
             */
            void playFrom(Page& page, int row, bool chooseService = false);

            /** Optional entries of trackContextMenu(). */
            enum TrackMenuFlags : unsigned {
                kTrackOpenRelease   = 1u << 0,
                kTrackOpenRecording = 1u << 1,
            };

            /**
             * The shared right-click menu for a track row: Play, save/unsave,
             * the optional "go to" entries, Edit, track number, Delete.
             * Refreshes the view when the data changed.
             */
            void trackContextMenu(Page& page, int row, unsigned flags);

            /** Navigate the main window to @p uri. */
            void open(const std::string& uri);

            Database* m_db = nullptr;
            std::vector<std::unique_ptr<Page>> m_pages;

        private:
            Page* pageFor(SPObject* sender);
            int onRowDblClicked(SPObject* sender, SPEventArgs* args);
            int onRowRightClicked(SPObject* sender, SPEventArgs* args);
            int onToolButtonClicked(SPObject* sender, SPEventArgs* args);
            // Repaints the tables when track availability arrives.
            service::AvailabilityResolver::Subscription m_availabilityChanged;
        };

        /**
         * "bungalow:library:<kind>:<key>" for the library's detail pages. The
         * plain bungalow:artist:<isni> form stays with the template-driven
         * artist page (views/artist.xml).
         */
        std::string libraryUri(const std::string& kind, const std::string& key);

        /** The key of a "bungalow:library:<kind>:<key>" URI, or "" if it doesn't match. */
        std::string libraryKey(const std::string& kind, const std::string& uri);

    } // namespace views
} // namespace space
