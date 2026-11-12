#include "app/accounts/AccountsView.h"

#include "SPPlaybackEventArgs.h"
#include "SPString.h"
#include "service/ServiceManager.h"

namespace space {
namespace app {

// ── AccountsDataSource ───────────────────────────────────────────────────────

AccountsDataSource::AccountsDataSource(service::ServiceManager* services) : m_services(services) {
    refresh();
}

void AccountsDataSource::refresh() {
    m_rows.clear();
    if (!m_services) return;
    for (service::Service* s : m_services->all()) {
        if (!s->supportsLogin()) continue;
        Row row{s->getId(), s->getName(), "", "Signed out"};
        if (auto session = m_services->accounts().get(s->getId())) {
            row.account = session->user().name;
            row.status = "Signed in";
        }
        m_rows.push_back(std::move(row));
    }
}

std::string AccountsDataSource::serviceIdForRow(int row) const {
    return row >= 0 && row < static_cast<int>(m_rows.size()) ? m_rows[row].serviceId : "";
}

string AccountsDataSource::getTextForColumn(int column) {
    static const char* kColumns[] = {"Service", "Account", "Status"};
    return column >= 0 && column < 3 ? kColumns[column] : "";
}

string AccountsDataSource::getTextForCell(int column, int row) {
    if (row < 0 || row >= static_cast<int>(m_rows.size())) return "";
    const Row& r = m_rows[row];
    switch (column) {
        case 0: return r.service;
        case 1: return r.account;
        case 2: return r.status;
    }
    return "";
}

// ── AccountsView ─────────────────────────────────────────────────────────────

AccountsView::AccountsView(SPElement* parent, service::ServiceManager* services)
    : SPContainerViewElement(parent), m_services(services) {
    this->setAttribute("src", "bungalow:accounts");

    m_source = new AccountsDataSource(services);
    m_table = new SPTableElement(this, m_source);
    m_table->fullView = true;
    m_table->setFlex(1);
    m_table->addEventListener("rowdblclicked",
                              reinterpret_cast<SPCallback>(&AccountsView::onRowDblClicked),
                              this, m_table);
    this->appendChild(m_table);

    // Sign-ins finish asynchronously; the manager reports each one here.
    if (m_services) m_services->accounts().setChangedCallback([this] { refresh(); });
}

AccountsView::~AccountsView() {
    if (m_services) m_services->accounts().setChangedCallback(nullptr);
}

bool AccountsView::acceptsUri(const std::string& uri) {
    return uri == "bungalow:accounts" || space::startsWith(uri, "bungalow:accounts#");
}

void AccountsView::navigate(const std::string& /*uri*/) { refresh(); }

void AccountsView::refresh() {
    m_source->refresh();
    m_table->invalidate();
}

int AccountsView::onRowDblClicked(SPObject* /*sender*/, SPEventArgs* args) {
    auto* rowArgs = dynamic_cast<SPTableRowEventArgs*>(args);
    if (!rowArgs || !m_services) return 0;

    const std::string id = m_source->serviceIdForRow(rowArgs->getRow());
    if (id.empty()) return 0;

    if (m_services->accounts().isSignedIn(id)) {
        m_services->logout(id); // the account callback refreshes the table
    } else {
        // Success is shown through the account callback. A failed attempt
        // leaves the row as it is.
        m_services->login(id, nullptr, nullptr);
    }
    return 0;
}

} // namespace app
} // namespace space
