#include "LocalFilesView.h"
#include "LocalFilesDataSource.h"
#include "PlayContext.h"
#include "SPTableElement.h"
#include "SPSectionElement.h"
#include "SPTabStackElement.h"
#include "SPTabBarViewElement.h"
#include "SPTabBarElement.h"
#include "SPPlaybackEventArgs.h"
#include "BungalowApp.h"

#include <regex>

namespace space {
    namespace views {

        LocalFilesView::LocalFilesView() : SPTabBarViewElement() {}

        LocalFilesView::LocalFilesView(SPElement* parent, sqlite3* db)
            : SPTabBarViewElement(parent)
        {
            auto* section = new SPSectionElement(this->tabStack);
            section->setId("overview");
            section->setStringAttribute("title", "Local Files");
            section->setFlex(1);

            m_dataSource = new LocalFilesDataSource(db);
            m_table = new SPTableElement(section, m_dataSource);
            m_table->fullView = true;
            m_table->setFlex(1);

            // Listen for double-click events on the table
            m_table->addEventListener(
                "rowdblclicked",
                reinterpret_cast<SPCallback>(&LocalFilesView::onTableDoubleClick),
                this, m_table);

            section->appendChild(m_table);

            this->tabStack->appendChild(section);
            this->tabBar->addTab("overview", "Local Files", "bungalow:localfiles");
            this->tabStack->setTabId("overview");
        }

        void LocalFilesView::navigate(const std::string& /*uri*/) {}

        void LocalFilesView::setHash(const std::string& hash) {
            SPTabBarViewElement::setHash(hash.empty() ? "overview" : hash);
        }

        bool LocalFilesView::acceptsUri(const std::string& uri) {
            return std::regex_match(uri, std::regex("bungalow:localfiles"));
        }

        int LocalFilesView::onTableDoubleClick(SPObject* /*sender*/, SPEventArgs* args) {
            auto* rowArgs = dynamic_cast<SPTableRowEventArgs*>(args);
            if (!rowArgs || !m_dataSource) return 0;

            auto* app = BungalowApp::current();
            if (!app) return 0;

            PlayContext ctx;
            ctx.entries    = m_dataSource->buildQueueEntries();
            ctx.startIndex = static_cast<std::size_t>(rowArgs->getRow());
            app->playContext(std::move(ctx));
            return 0;
        }

        LocalFilesView::~LocalFilesView() {}

    } // namespace views
} // namespace space
