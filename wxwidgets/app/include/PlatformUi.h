#pragma once

#include <sqlite3.h>

#include <string>
#include <utility>
#include <vector>

namespace space {
namespace platform {

using MenuItems = std::vector<std::pair<int, std::string>>;

// Shows a popup menu at the cursor and returns the chosen item id, or -1 if dismissed.
// An item with an empty label is a separator.
int showPopupMenu(const MenuItems& items);

// One labelled text field of showFormDialog().
struct FormField {
    std::string label;
    std::string value;          // initial text in, entered (trimmed) text out
    std::string hint;           // placeholder or short explanation; may be empty
    bool        required = false;
    bool        readOnly = false;
};

// Shows one labelled text field per entry. OK is refused while a required field
// is empty. Returns true if the user pressed OK; the fields' values are updated.
bool showFormDialog(const std::string& title, std::vector<FormField>& fields);

// Shows a list to pick one entry from. Returns the chosen index, or -1 if cancelled.
int showChoiceDialog(const std::string& title, const std::string& message,
                     const std::vector<std::string>& choices);

// Shows the add (trackId == 0) or edit track dialog. Returns true if changes were saved.
bool showAddTrackDialog(sqlite3* db, int trackId = 0);

// Shows the dialog for defining a new user table. Returns true if a table was created.
bool showCreateTableDialog(sqlite3* db);

// Shows the add (rowUuid empty) or edit row dialog for a user table. Returns true if saved.
bool showCustomRowDialog(sqlite3* db, const std::string& tableName, const std::string& rowUuid = "");

// Shows a yes/no warning prompt. Returns true if the user confirmed.
bool showConfirmDialog(const std::string& title, const std::string& message);

// Shows a folder picker. Returns the chosen path, or "" if cancelled.
std::string showFolderDialog(const std::string& title, const std::string& initialPath);

// Per-user application data directory (created if missing).
std::string userDataDir();

std::string homeDir();

// The user's Documents folder.
std::string documentsDir();

} // namespace platform
} // namespace space
