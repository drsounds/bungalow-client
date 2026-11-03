#include "Database.h"
#include "playlist/RootlistEvents.h"
#include "playlist/RootlistManager.h"

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

using space::playlist::RootlistEvents;
using space::playlist::RootlistManager;

namespace {

int failures = 0;

void expect(const std::string& actual, const std::string& expected, const char* what) {
    if (actual == expected) return;
    std::cerr << what << ": expected \"" << expected << "\", got \"" << actual << "\"\n";
    ++failures;
}

void expect(int actual, int expected, const char* what) {
    expect(std::to_string(actual), std::to_string(expected), what);
}

// Titles top first, joined with commas; also checks positions are 0..n-1.
std::string order(Database& db) {
    std::string result;
    const auto nodes = RootlistManager::nodes(db);
    for (size_t i = 0; i < nodes.size(); ++i) {
        expect(nodes[i].position, static_cast<int>(i), "contiguous positions");
        if (!result.empty()) result += ",";
        result += nodes[i].title;
    }
    return result;
}

} // namespace

int main() {
    const auto path = std::filesystem::temp_directory_path() / "bungalow-rootlist-test.db";
    std::filesystem::remove(path);
    {
        Database db(path.string());
        int changes = 0;
        const int listener = RootlistEvents::addListener([&changes] { ++changes; });

        const std::string a = RootlistManager::createPlaylist(db, "A");
        const std::string b = RootlistManager::createPlaylist(db, "B");
        const std::string c = RootlistManager::createPlaylist(db, "C");
        expect(order(db), "C,B,A", "new playlists go on top");
        expect(changes, 3, "create fires changes");
        expect(a.rfind(RootlistManager::kPlaylistPrefix, 0) == 0 ? 1 : 0, 1, "playlist URI prefix");
        expect(db.getPlaylistName(RootlistManager::playlistId(b)), "B", "playlist row created");

        RootlistManager::move(db, c, 2);
        expect(order(db), "B,A,C", "move down");
        RootlistManager::move(db, c, 0);
        expect(order(db), "C,B,A", "move up");
        RootlistManager::move(db, a, 1);
        expect(order(db), "C,A,B", "move up by one");
        RootlistManager::move(db, b, 99);
        expect(order(db), "C,A,B", "move past the end clamps");

        RootlistManager::rename(db, a, "Renamed");
        expect(order(db), "C,Renamed,B", "rename node");
        expect(db.getPlaylistName(RootlistManager::playlistId(a)), "Renamed", "rename playlist");

        RootlistManager::remove(db, c);
        expect(order(db), "Renamed,B", "remove closes the gap");
        expect(db.getPlaylistName(RootlistManager::playlistId(c)), "", "remove deletes playlist");

        expect(db.insertRootlistNode(b, "Duplicate", 0) ? 1 : 0, 0, "a URI is listed once");
        expect(RootlistManager::playlistId("bungalow:playlist:x:y"), "", "playlistId rejects sub-URIs");
        expect(RootlistManager::playlistId("bungalow:site:x"), "", "playlistId rejects other URIs");

        RootlistEvents::removeListener(listener);
    }
    std::filesystem::remove(path);

    if (failures == 0) std::cout << "All rootlist tests passed\n";
    return failures == 0 ? 0 : 1;
}
