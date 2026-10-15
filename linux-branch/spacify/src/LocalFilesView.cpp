#include "LocalFilesView.h"
#include "../../space/src/SPPageElement.h"
#include "../../space/src/SPPlaybackEventArgs.h"
#include "../../space/src/SPTabBarElement.h"
#include "../../space/src/SPTabBarViewElement.h"
#include "../../space/src/SPTabStackElement.h"
#include "../../space/src/SPTableElement.h"
#include "LocalFilesDataSource.h"
#include "PlayContext.h"
#include "SpacifyApp.h"

#include <regex>
#include <wx/app.h>

namespace space {
namespace views {

LocalFilesView::LocalFilesView() :
	SPTabBarViewElement() {}

LocalFilesView::LocalFilesView(SPElement *parent, sqlite3 *db) :
	SPTabBarViewElement(parent) {
	auto *page = new SPPageElement(this->tabStack);
	page->setId("overview");
	page->setStringAttribute("title", "Local Files");
	page->setFlex(1);

	m_dataSource = new LocalFilesDataSource(db);
	m_table = new SPTableElement(page, m_dataSource);
	m_table->fullView = true;
	m_table->setFlex(1);

	// Listen for double-click events on the table
	m_table->addEventListener(
		"rowdblclicked",
		reinterpret_cast<SPCallback>(&LocalFilesView::onTableDoubleClick), this,
		m_table);

	page->appendChild(m_table);

	this->tabStack->appendChild(page);
	this->tabBar->addTab("overview", "Local Files", "spacify:localfiles");
	this->tabStack->setTabId("overview");
}

void LocalFilesView::navigate(const std::string & /*uri*/) {}

void LocalFilesView::setHash(const std::string &hash) {
	SPTabBarViewElement::setHash(hash.empty() ? "overview" : hash);
}

bool LocalFilesView::acceptsUri(const std::string &uri) {
	return std::regex_match(uri, std::regex("spacify:localfiles"));
}

int LocalFilesView::onTableDoubleClick(SPObject * /*sender*/,
									   SPEventArgs *args) {
	auto *rowArgs = dynamic_cast<SPTableRowEventArgs *>(args);
	if (!rowArgs || !m_dataSource)
		return 0;

	auto *app = dynamic_cast<SpacifyApp *>(wxTheApp);
	if (!app)
		return 0;

	PlayContext ctx;
	ctx.entries = m_dataSource->buildQueueEntries();
	ctx.startIndex = static_cast<std::size_t>(rowArgs->getRow());
	app->playContext(std::move(ctx));
	return 0;
}

LocalFilesView::~LocalFilesView() {}

} // namespace views
} // namespace space
