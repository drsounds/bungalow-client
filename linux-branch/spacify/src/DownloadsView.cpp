#include "DownloadsView.h"
#include "../../space/src/SPPageElement.h"
#include "../../space/src/SPPlaybackEventArgs.h"
#include "../../space/src/SPTabBarElement.h"
#include "../../space/src/SPTabStackElement.h"
#include <boost/algorithm/string/predicate.hpp>

namespace space {
namespace views {

DownloadsView::DownloadsView() :
	SPTabBarViewElement() {}

DownloadsView::DownloadsView(SPElement *parent, DownloadManager *dm) :
	SPTabBarViewElement(parent), m_dm(dm) {

	// ── Ongoing tab ────────────────────────────────────────────────────────
	auto *ongoingPage = new SPPageElement(this->tabStack);
	ongoingPage->setId("overview");
	ongoingPage->setStringAttribute("title", "Ongoing");
	ongoingPage->setFlex(1);

	m_ongoingDS =
		new DownloadsDataSource(dm, DownloadsDataSource::Filter::Ongoing);
	m_ongoingTable = new SPTableElement(ongoingPage, m_ongoingDS);
	m_ongoingTable->fullView = true;
	m_ongoingTable->setFlex(1);
	m_ongoingTable->addEventListener(
		"rowdblclicked",
		reinterpret_cast<SPCallback>(&DownloadsView::onOngoingRowDblClicked),
		this, m_ongoingTable);
	ongoingPage->appendChild(m_ongoingTable);
	this->tabStack->appendChild(ongoingPage);

	// ── Completed tab ──────────────────────────────────────────────────────
	auto *completedPage = new SPPageElement(this->tabStack);
	completedPage->setId("completed");
	completedPage->setStringAttribute("title", "Completed");
	completedPage->setFlex(1);

	m_completedDS =
		new DownloadsDataSource(dm, DownloadsDataSource::Filter::Completed);
	m_completedTable = new SPTableElement(completedPage, m_completedDS);
	m_completedTable->fullView = true;
	m_completedTable->setFlex(1);
	m_completedTable->addEventListener(
		"rowdblclicked",
		reinterpret_cast<SPCallback>(&DownloadsView::onCompletedRowDblClicked),
		this, m_completedTable);
	completedPage->appendChild(m_completedTable);
	this->tabStack->appendChild(completedPage);

	// ── Tab bar ────────────────────────────────────────────────────────────
	this->tabBar->addTab("overview", "Ongoing", "spacify:downloads");
	this->tabBar->addTab("completed", "Completed",
						 "spacify:downloads#completed");
	this->tabStack->setTabId("overview");

	// ── Register update callback with DownloadManager ──────────────────────
	if (dm) {
		dm->setUpdateCallback([this] { refresh(); });
	}
}

DownloadsView::~DownloadsView() {
	// Remove our callback so DownloadManager doesn't call into a dead view
	if (m_dm)
		m_dm->setUpdateCallback(nullptr);
}

bool DownloadsView::acceptsUri(const std::string &uri) {
	return uri == "spacify:downloads" ||
		   boost::algorithm::starts_with(uri, "spacify:downloads#");
}

void DownloadsView::navigate(const std::string & /*uri*/) {}

void DownloadsView::setHash(const std::string &hash) {
	SPTabBarViewElement::setHash(hash.empty() ? "ongoing" : hash);
}

void DownloadsView::refresh() {
	if (m_ongoingDS) {
		m_ongoingDS->refresh();
		m_ongoingTable->invalidate();
	}
	if (m_completedDS) {
		m_completedDS->refresh();
		m_completedTable->invalidate();
	}
}

// ── Row double-click: cancel ongoing / open completed ────────────────────────

int DownloadsView::onOngoingRowDblClicked(SPObject * /*sender*/,
										  SPEventArgs *args) {
	auto *rowArgs = dynamic_cast<SPTableRowEventArgs *>(args);
	if (!rowArgs || !m_ongoingDS)
		return 0;

	int64_t itemId = m_ongoingDS->getItemIdForRow(rowArgs->getRow());
	if (itemId <= 0)
		return 0;

	if (m_dm)
		m_dm->cancelDownload(itemId);

	return 0;
}

int DownloadsView::onCompletedRowDblClicked(SPObject * /*sender*/,
											SPEventArgs * /*args*/) {
	// Could open the file in Finder — left for a future enhancement
	return 0;
}

} // namespace views
} // namespace space
