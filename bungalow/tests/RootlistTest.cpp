#include "Database.h"
#include "playlist/RootlistEvents.h"
#include "playlist/RootlistManager.h"
#include "playlist/Upsl.h"

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

using space::playlist::RootlistEvents;
using space::playlist::RootlistManager;
using space::playlist::Upsl;
namespace base62 = space::playlist::base62;

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

void testUpsl() {
    expect(base62::encode("Hello"), "5TP3P3v", "base62 encode");
    expect(base62::decode("5TP3P3v").value_or("?"), "Hello", "base62 decode");
    const std::string zeros("\0\0ab", 4);
    expect(base62::decode(base62::encode(zeros)).value_or("?") == zeros ? 1 : 0, 1, "base62 keeps leading zeros");
    expect(base62::decode("abc-").has_value() ? 1 : 0, 0, "base62 rejects other characters");

    const std::string handle = "@drsounds@mastodon.social";
    const std::string id = "8f2b1c3d-0000-4000-8000-000000000001";
    const auto hosted = Upsl::parse(Upsl::hostedUri(handle, id));
    expect(hosted ? 1 : 0, 1, "parse hosted");
    if (hosted) {
        expect(hosted->handle, handle, "hosted handle");
        expect(hosted->playlistId, id, "hosted id");
        expect(hosted->embedded ? 1 : 0, 0, "hosted has no body");
    }

    Upsl shared;
    shared.handle = handle;
    shared.playlistId = id;
    shared.name = "Sommar: 2026, mix";
    shared.description = "";
    shared.refs = {"musik:isrc:GBAYE0601498", "musik:artist:The%20Band:release:R:name:Song", "https://example.com/v"};
    const auto embedded = Upsl::parse(shared.embeddedUri() + ":sig:abc_-");
    expect(embedded ? 1 : 0, 1, "parse embedded");
    if (embedded) {
        expect(embedded->embedded ? 1 : 0, 1, "embedded has a body");
        expect(embedded->name, shared.name, "embedded name");
        expect(embedded->description, "", "embedded empty description");
        expect(static_cast<int>(embedded->refs.size()), 3, "embedded refs");
        if (embedded->refs.size() == 3) expect(embedded->refs[1], shared.refs[1], "embedded ref");
    }

    const std::string legacy = "bungalow:federated:user:" + handle + ":playlist:" + id;
    expect(Upsl::isUpsl(legacy) ? 1 : 0, 1, "legacy prefix accepted");
    expect(Upsl::canonicalUri(legacy), Upsl::hostedUri(handle, id), "legacy prefix canonicalised");
    expect(Upsl::isUpsl("musik:isrc:X") ? 1 : 0, 0, "content ref is not a UPSL");
    expect(Upsl::parse("musik:federated:user:@a@b:nope").has_value() ? 1 : 0, 0, "missing :playlist: rejected");

    expect(Upsl::isValidHandle(handle) ? 1 : 0, 1, "valid handle");
    expect(Upsl::isValidHandle("drsounds@mastodon.social") ? 1 : 0, 0, "handle needs leading @");
    expect(Upsl::isValidHandle("@drsounds") ? 1 : 0, 0, "handle needs a domain");
    expect(Upsl::isValidHandle("@a:b@c") ? 1 : 0, 0, "handle rejects ':'");
    expect(Upsl::isValidHandle(Upsl::defaultHandle()) ? 1 : 0, 1, "default handle is valid");
    expect(space::playlist::percentDecode("The%20Band%3A"), "The Band:", "percentDecode");
}

} // namespace

int main() {
    testUpsl();

    const auto path = std::filesystem::temp_directory_path() / "bungalow-rootlist-test.db";
    std::filesystem::remove(path);
    {
        Database db(path.string());
        int changes = 0;
        const int listener = RootlistEvents::addListener([&changes] { ++changes; });
        const std::string me = "@me@example.social";

        const std::string a = RootlistManager::createPlaylist(db, "A", me);
        const std::string b = RootlistManager::createPlaylist(db, "B", me);
        const std::string c = RootlistManager::createPlaylist(db, "C", me);
        expect(order(db), "C,B,A", "new playlists go on top");
        expect(changes, 3, "create fires changes");
        expect(a.rfind("musik:federated:user:" + me + ":playlist:", 0) == 0 ? 1 : 0, 1, "federated playlist URI");
        expect(db.getPlaylistName(RootlistManager::playlistId(b)), "B", "playlist row created");
        expect(RootlistManager::isLocalPlaylist(db, b) ? 1 : 0, 1, "listed playlist is local");
        expect(RootlistManager::isLocalPlaylist(
                   db, Upsl::hostedUri("@other@example.social", RootlistManager::playlistId(b))) ? 1 : 0,
               0, "another hosted link is not local");
        expect(db.insertPlaylistRefs(RootlistManager::playlistId(b),
                                     {"musik:isrc:ONE", "musik:upc:TWO"}, 0) ? 1 : 0,
               1, "insert playlist content");
        std::vector<std::string> refs;
        expect(db.getPlaylistRefs(RootlistManager::playlistId(b), refs) ? 1 : 0,
               1, "read playlist content successfully");
        expect(static_cast<int>(refs.size()), 2, "read playlist content");
        if (refs.size() == 2) {
            expect(refs[0], "musik:isrc:ONE", "playlist content first");
            expect(refs[1], "musik:upc:TWO", "playlist content second");
        }
        expect(db.insertPlaylistRefs(RootlistManager::playlistId(b),
                                     {"musik:isni:THREE"}, 1) ? 1 : 0,
               1, "insert playlist content at position");
        expect(db.getPlaylistRefs(RootlistManager::playlistId(b), refs) ? 1 : 0,
               1, "read inserted playlist content successfully");
        if (refs.size() == 3)
            expect(refs[1], "musik:isni:THREE", "playlist content insertion position");
        expect(RootlistManager::insertContentRefs(db, b, {"musik:isrc:FOUR"}, 3) ? 1 : 0,
               1, "insert content through playlist manager");

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
        expect(RootlistManager::playlistId("bungalow:site:x"), "", "playlistId rejects other URIs");

        // Playlists from before federation keep their place under a federated link.
        const std::string oldId = db.createPlaylist("Old", "");
        db.insertRootlistNode("bungalow:playlist:" + oldId, "Old", 1);
        RootlistManager::migrateLegacyUris(db, me);
        const auto nodes = RootlistManager::nodes(db);
        expect(nodes.size() == 3 ? nodes[1].uri : std::string(), Upsl::hostedUri(me, oldId), "legacy URI migrated");
        expect(order(db), "Renamed,Old,B", "migration keeps the order");

        RootlistEvents::removeListener(listener);
    }
    std::filesystem::remove(path);

    if (failures == 0) std::cout << "All rootlist tests passed\n";
    return failures == 0 ? 0 : 1;
}
