#include "LibraryView.h"
#include "LibraryDataSource.h"
#include "PlayContext.h"
#include "SPTableElement.h"
#include "SPSectionElement.h"
#include "SPTabStackElement.h"
#include "SPTabBarViewElement.h"
#include "SPTabBarElement.h"
#include "SPPlaybackEventArgs.h"
#include "BungalowApp.h"
#include "SPColor.h"
#include "PlatformUi.h"

#include <regex>

namespace space {
    namespace views {

        LibraryView::LibraryView() : SPTabBarViewElement() {}

        LibraryView::LibraryView(SPElement* parent, sqlite3* db)
            : SPTabBarViewElement(parent)
        {
            auto* section = new SPSectionElement(this->tabStack);
            section->setId("overview");
            section->setStringAttribute("title", "Library");
            section->setFlex(1);

            m_db = db;
            m_dataSource = new LibraryDataSource(db);
            m_table = new SPTableElement(section, m_dataSource);
            m_table->fullView = true;
            section->appendChild(m_table);
            m_table->setFlex(1);

            // Listen for double-click on any row
            m_table->addEventListener(
                "rowdblclicked",
                reinterpret_cast<SPCallback>(&LibraryView::onRowDblClicked),
                this,
                m_table
            );

            // Listen for right-click on any row
            m_table->addEventListener(
                "rowrightclicked",
                reinterpret_cast<SPCallback>(&LibraryView::onRowRightClicked),
                this,
                m_table
            );

            this->tabStack->appendChild(section);
            this->tabBar->addTab("overview", "Library", "bungalow:library");
            this->tabStack->setTabId("overview");

            // + tool button on the right of the tab bar
            this->tabBar->addToolButton("add", "+");
            this->tabBar->addEventListener(
                "toolbuttonclicked",
                reinterpret_cast<SPCallback>(&LibraryView::onToolButtonClicked),
                this,
                this->tabBar
            );
        }

        void LibraryView::playRowInContext(int row) {
            auto* app = BungalowApp::current();
            if (!app || !m_dataSource || !m_db) return;

            PlayContext ctx;
            ctx.entries    = m_dataSource->buildQueueEntries(m_db);
            ctx.startIndex = static_cast<std::size_t>(row);
            app->playContext(std::move(ctx));
        }

        int LibraryView::onRowDblClicked(SPObject* /*sender*/, SPEventArgs* args) {
            auto* rowArgs = dynamic_cast<SPTableRowEventArgs*>(args);
            if (!rowArgs) return 0;

            playRowInContext(rowArgs->getRow());
            return 0;
        }

        int LibraryView::onToolButtonClicked(SPObject* /*sender*/, SPEventArgs* args) {
            auto* btnArgs = dynamic_cast<space::TabBarToolButtonEventArgs*>(args);
            if (!btnArgs || btnArgs->buttonId != "add") return 0;
            if (!m_db) return 0;

            if (platform::showAddTrackDialog(m_db)) {
                if (m_dataSource) m_dataSource->reload(m_db);
                if (m_table)      m_table->invalidate();
            }
            return 0;
        }

        int LibraryView::onRowRightClicked(SPObject* /*sender*/, SPEventArgs* args) {
            auto* rowArgs = dynamic_cast<SPTableRowEventArgs*>(args);
            if (!rowArgs || !m_dataSource) return 0;

            const int trackId = m_dataSource->getTrackIdForRow(rowArgs->getRow());
            if (trackId <= 0) return 0;

            enum { ID_PLAY = 10100, ID_EDIT };
            const int sel = platform::showPopupMenu({ { ID_PLAY, "Play" }, { ID_EDIT, "Edit..." } });

            if (sel == ID_PLAY) {
                playRowInContext(rowArgs->getRow());
            } else if (sel == ID_EDIT) {
                if (platform::showAddTrackDialog(m_db, trackId)) {
                    if (m_dataSource) m_dataSource->reload(m_db);
                    if (m_table)      m_table->invalidate();
                }
            }
            return 0;
        }

        void LibraryView::navigate(const std::string& /*uri*/) {
            if (m_dataSource && m_db) {
                m_dataSource->reload(m_db);
                if (m_table) m_table->invalidate();
            }
        }

        void LibraryView::setHash(const std::string& hash) {
            SPTabBarViewElement::setHash(hash.empty() ? "overview" : hash);
        }

        bool LibraryView::acceptsUri(const std::string& uri) {
            return std::regex_match(uri, std::regex("bungalow:library"));
        }

        LibraryView::~LibraryView() {}

    } // namespace views
} // namespace space
