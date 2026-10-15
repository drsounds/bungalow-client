#pragma once

#include <sqlite3.h>

#include <string>
#include <utility>
#include <vector>

namespace space {
namespace platform {

using MenuItems = std::vector<std::pair<int, std::string>>;

// Shows a popup menu at the cursor and returns the chosen item id, or -1 if dismissed.
int showPopupMenu(const MenuItems& items);

// Shows the add (trackId == 0) or edit track dialog. Returns true if changes were saved.
bool showAddTrackDialog(sqlite3* db, int trackId = 0);

// Shows a folder picker. Returns the chosen path, or "" if cancelled.
std::string showFolderDialog(const std::string& title, const std::string& initialPath);

// Per-user application data directory (created if missing).
std::string userDataDir();

std::string homeDir();

} // namespace platform
} // namespace space
