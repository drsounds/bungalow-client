#pragma once

#include <sqlite3.h>

class Database;
class DownloadManager;

namespace space {
namespace service { class ServiceManager; }

namespace app {

/// Services of the running BungalowApp that built-in features construct
/// their views with. Null before the app has opened them.
Database* database();
sqlite3* databaseHandle();
DownloadManager* downloadManager();
service::ServiceManager* serviceManager();

} // namespace app
} // namespace space
