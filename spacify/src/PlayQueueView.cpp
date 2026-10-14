#include "PlayQueueView.h"
#include "PlayQueueDataSource.h"
#include "SPTableElement.h"
#include "SPSectionElement.h"
#include "SPTabStackElement.h"
#include "SpacifyApp.h"
#include "SPTabBarElement.h"
#include "SPPlaybackEventArgs.h"

#include <wx/app.h>

namespace space {
namespace views {

PlayQueueView::PlayQueueView() : SPTabBarViewElement() {}

PlayQueueView::PlayQueueView(SPElement* parent)
    : SPTabBarViewElement(parent)
{
    // Override the default flex=1 from SPTabBarViewElement so the panel
    // keeps a fixed width when used as a sidebar column.
    this->setFlex(0);
    this->setWidth(240);
    this->setColorScheme(SP_COLOR_SCHEME_DARK);

    auto* section = new SPSectionElement(this->tabStack);
    section->setId("overview");
    section->setStringAttribute("title", "Play queue");
    section->setFlex(1);

    m_dataSource = new PlayQueueDataSource();
    m_table = new SPTableElement(section, m_dataSource);
    m_table->fullView = true;
    m_table->gradientView = true;
    m_table->setFlex(1);
    section->appendChild(m_table);

    // Double-click a row → play that entry from the existing queue
    m_table->addEventListener(
        "rowdblclicked",
        reinterpret_cast<SPCallback>(&PlayQueueView::onRowDblClicked),
        this,
        m_table
    );

    this->tabStack->appendChild(section);
    this->tabBar->addTab("overview", "Play queue", "spacify:internal:playqueue");
    this->tabStack->setTabId("overview");

    // Subscribe to queue and playback events so the panel stays in sync.
    if (auto* app = dynamic_cast<SpacifyApp*>(wxTheApp)) {
        app->getPlaybackEvents().addEventListener(
            "queue:changed",
            reinterpret_cast<SPCallback>(&PlayQueueView::reload),
            this, nullptr);

        app->getPlaybackEvents().addEventListener(
            "playback:changed",
            reinterpret_cast<SPCallback>(&PlayQueueView::reload),
            this, nullptr);
    }

    // Populate immediately with the current (possibly empty) queue.
    reload(nullptr, nullptr);
}

int PlayQueueView::reload(SPObject* /*sender*/, SPEventArgs* /*args*/) {
    if (!m_dataSource) return 0;
    if (auto* app = dynamic_cast<SpacifyApp*>(wxTheApp)) {
        m_dataSource->setEntries(app->getQueueEntries(), app->getQueueIndex());
    }
    if (m_table) m_table->invalidate();
    return 0;
}

int PlayQueueView::onRowDblClicked(SPObject* /*sender*/, SPEventArgs* args) {
    auto* rowArgs = dynamic_cast<SPTableRowEventArgs*>(args);
    if (!rowArgs) return 0;

    if (auto* app = dynamic_cast<SpacifyApp*>(wxTheApp))
        app->playQueueAtIndex(rowArgs->getRow());

    return 0;
}

PlayQueueView::~PlayQueueView() {}

} // namespace views
} // namespace space
