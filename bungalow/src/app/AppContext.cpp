#include "app/AppContext.h"

#include "BungalowApp.h"
#include "Database.h"

namespace space {
namespace app {

Database* database() {
    auto* app = BungalowApp::current();
    return app ? app->getDatabase() : nullptr;
}

sqlite3* databaseHandle() {
    Database* db = database();
    return db ? db->handle() : nullptr;
}

DownloadManager* downloadManager() {
    auto* app = BungalowApp::current();
    return app ? app->getDownloadManager() : nullptr;
}

service::ServiceManager* serviceManager() {
    auto* app = BungalowApp::current();
    return app ? &app->getServiceManager() : nullptr;
}

} // namespace app
} // namespace space
