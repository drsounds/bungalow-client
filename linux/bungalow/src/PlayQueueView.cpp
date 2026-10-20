#include "PlayQueueView.h"
#include "../../space/src/SPPageElement.h"
#include "../../space/src/SPPlaybackEventArgs.h"
#include "../../space/src/SPTabBarElement.h"
#include "../../space/src/SPTabStackElement.h"
#include "../../space/src/SPTableElement.h"
#include "PlayQueueDataSource.h"
#include "BungalowApp.h"

#include <wx/app.h>

namespace space {
namespace views {

PlayQueueView::PlayQueueView() :
	SPTabBarViewElement() {}

PlayQueueView::PlayQueueView(SPElement *parent) :
	SPTabBarViewElement(parent) {
	// Override the default flex=1 from SPTabBarViewElement so the panel
	// keeps a fixed width when used as a sidebar column.
	this->setFlex(0);
	this->setWidth(240);
	this->setColorScheme(SP_COLOR_SCHEME_DARK);

	auto *page = new SPPageElement(this->tabStack);
	page->setId("overview");
	page->setStringAttribute("title", "Play queue");
	page->setFlex(1);

	m_dataSource = new PlayQueueDataSource();
	m_table = new SPTableElement(page, m_dataSource);
	m_table->fullView = true;
	m_table->gradientView = true;
	m_table->setFlex(1);
	page->appendChild(m_table);

	// Double-click a row → play that entry from the existing queue
	m_table->addEventListener(
		"rowdblclicked",
		reinterpret_cast<SPCallback>(&PlayQueueView::onRowDblClicked), this,
		m_table);

	this->tabStack->appendChild(page);
	this->tabBar->addTab("overview", "Play queue",
						 "bungalow:internal:playqueue");
	this->tabStack->setTabId("overview");

	// Subscribe to queue and playback events so the panel stays in sync.
	if (auto *app = dynamic_cast<BungalowApp *>(wxTheApp)) {
		app->getPlaybackEvents().addEventListener(
			"queue:changed",
			reinterpret_cast<SPCallback>(&PlayQueueView::reload), this,
			nullptr);

		app->getPlaybackEvents().addEventListener(
			"playback:changed",
			reinterpret_cast<SPCallback>(&PlayQueueView::reload), this,
			nullptr);
	}

	// Populate immediately with the current (possibly empty) queue.
	reload(nullptr, nullptr);
}

int PlayQueueView::reload(SPObject * /*sender*/, SPEventArgs * /*args*/) {
	if (!m_dataSource)
		return 0;
	if (auto *app = dynamic_cast<BungalowApp *>(wxTheApp)) {
		m_dataSource->setEntries(app->getQueueEntries(), app->getQueueIndex());
	}
	if (m_table)
		m_table->invalidate();
	return 0;
}

int PlayQueueView::onRowDblClicked(SPObject * /*sender*/, SPEventArgs *args) {
	auto *rowArgs = dynamic_cast<SPTableRowEventArgs *>(args);
	if (!rowArgs)
		return 0;

	if (auto *app = dynamic_cast<BungalowApp *>(wxTheApp))
		app->playQueueAtIndex(rowArgs->getRow());

	return 0;
}

PlayQueueView::~PlayQueueView() {}

} // namespace views
} // namespace space
