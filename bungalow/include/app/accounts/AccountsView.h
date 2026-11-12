#pragma once

#include "SPContainerViewElement.h"
#include "SPTableDataSource.h"
#include "SPTableElement.h"

#include <string>
#include <vector>

namespace space {
namespace service { class ServiceManager; }

namespace app {

/// One row per service that has a sign-in: its account name and state.
class AccountsDataSource : public SPTableDataSource {
public:
    explicit AccountsDataSource(service::ServiceManager* services);

    void refresh();
    /// Id of the service shown in `row`, or "".
    std::string serviceIdForRow(int row) const;

    int    getColumnCount() override { return 3; }
    int    getRowCount() override { return static_cast<int>(m_rows.size()); }
    string getTextForColumn(int column) override;
    string getTextForCell(int column, int row) override;

private:
    struct Row { std::string serviceId, service, account, status; };
    service::ServiceManager* m_services;
    std::vector<Row>         m_rows;
};

/// bungalow:accounts. Double-click a row to sign in or out.
class AccountsView : public SPContainerViewElement {
public:
    AccountsView(SPElement* parent, service::ServiceManager* services);
    ~AccountsView() override;

    bool acceptsUri(const std::string& uri) override;
    void navigate(const std::string& uri) override;
    std::string getType() override { return "accountsview"; }

    void refresh();

private:
    int onRowDblClicked(SPObject* sender, SPEventArgs* args);

    service::ServiceManager* m_services;
    AccountsDataSource*      m_source = nullptr;
    SPTableElement*          m_table  = nullptr;
};

} // namespace app
} // namespace space
