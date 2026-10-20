#include "DownloadsView.h"
#include "SPSectionElement.h"
#include "SPTabStackElement.h"
#include "SPTabBarElement.h"
#include "SPPlaybackEventArgs.h"
#include "SPString.h"

namespace space {
namespace views {

DownloadsView::DownloadsView() : SPTabBarViewElement() {}

DownloadsView::DownloadsView(SPElement* parent, DownloadManager* dm)
    : SPTabBarViewElement(parent), m_dm(dm)
{

    // ── Ongoing tab ────────────────────────────────────────────────────────
    auto* ongoingSection = new SPSectionElement(this->tabStack);
    ongoingSection->setId("overview");
    ongoingSection->setStringAttribute("title", "Ongoing");
    ongoingSection->setFlex(1);

    m_ongoingDS    = new DownloadsDataSource(dm, DownloadsDataSource::Filter::Ongoing);
    m_ongoingTable = new SPTableElement(ongoingSection, m_ongoingDS);
    m_ongoingTable->fullView = true;
    m_ongoingTable->setFlex(1);
    m_ongoingTable->addEventListener(
        "rowdblclicked",
        reinterpret_cast<SPCallback>(&DownloadsView::onOngoingRowDblClicked),
        this, m_ongoingTable);
    ongoingSection->appendChild(m_ongoingTable);
    this->tabStack->appendChild(ongoingSection);

    // ── Completed tab ──────────────────────────────────────────────────────
    auto* completedSection = new SPSectionElement(this->tabStack);
    completedSection->setId("completed");
    completedSection->setStringAttribute("title", "Completed");
    completedSection->setFlex(1);

    m_completedDS    = new DownloadsDataSource(dm, DownloadsDataSource::Filter::Completed);
    m_completedTable = new SPTableElement(completedSection, m_completedDS);
    m_completedTable->fullView = true;
    m_completedTable->setFlex(1);
    m_completedTable->addEventListener(
        "rowdblclicked",
        reinterpret_cast<SPCallback>(&DownloadsView::onCompletedRowDblClicked),
        this, m_completedTable);
    completedSection->appendChild(m_completedTable);
    this->tabStack->appendChild(completedSection);

    // ── Tab bar ────────────────────────────────────────────────────────────
    this->tabBar->addTab("overview",   "Ongoing",   "bungalow:downloads");
    this->tabBar->addTab("completed", "Completed", "bungalow:downloads#completed");
    this->tabStack->setTabId("overview");

    // ── Register update callback with DownloadManager ──────────────────────
    if (dm) {
        dm->setUpdateCallback([this] { refresh(); });
    }
}

DownloadsView::~DownloadsView() {
    // Remove our callback so DownloadManager doesn't call into a dead view
    if (m_dm) m_dm->setUpdateCallback(nullptr);
}

bool DownloadsView::acceptsUri(const std::string& uri) {
    return uri == "bungalow:downloads" ||
           space::startsWith(uri, "bungalow:downloads#");
}

void DownloadsView::navigate(const std::string& /*uri*/) {}

void DownloadsView::setHash(const std::string& hash) {
    SPTabBarViewElement::setHash(hash.empty() ? "ongoing" : hash);
}

void DownloadsView::refresh() {
    if (m_ongoingDS)   { m_ongoingDS->refresh();   m_ongoingTable->invalidate(); }
    if (m_completedDS) { m_completedDS->refresh(); m_completedTable->invalidate(); }
}

// ── Row double-click: cancel ongoing / open completed ────────────────────────

int DownloadsView::onOngoingRowDblClicked(SPObject* /*sender*/, SPEventArgs* args) {
    auto* rowArgs = dynamic_cast<SPTableRowEventArgs*>(args);
    if (!rowArgs || !m_ongoingDS) return 0;

    int64_t itemId = m_ongoingDS->getItemIdForRow(rowArgs->getRow());
    if (itemId <= 0) return 0;

    if (m_dm) m_dm->cancelDownload(itemId);

    return 0;
}

int DownloadsView::onCompletedRowDblClicked(SPObject* /*sender*/, SPEventArgs* /*args*/) {
    // Could open the file in Finder — left for a future enhancement
    return 0;
}

} // namespace views
} // namespace space
