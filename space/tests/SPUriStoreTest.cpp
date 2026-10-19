#include "SPUriStore.h"

#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using space::SPUriEntry;
using space::SPUriStatus;
using space::SPUriStore;

namespace {

int failures = 0;

void check(bool condition, const char *message) {
	if (!condition) {
		std::cerr << "FAILED: " << message << "\n";
		++failures;
	}
}

fs::path makeTempDir() {
	fs::path dir = fs::temp_directory_path() / "spacify-uri-store-test";
	fs::remove_all(dir);
	fs::create_directories(dir);
	return dir;
}

} // namespace

int main() {
	const fs::path dir = makeTempDir();
	int64_t now = 1000;
	int fetches = 0;
	std::vector<std::function<void()>> deferred;
	std::vector<SPUriStore::ResolveCallback> pending;

	// Fresh fetch through a synchronous resolver is persisted to disk.
	{
		SPUriStore store;
		store.setClock([&] { return now; });
		store.setPersistenceDirectory(dir.u8string());
		store.registerResolver("spacify:artist:", [&](const std::string &uri, auto resolve, auto) {
			++fetches;
			resolve({{"uri", uri}, {"name", "Kraftwerk"}});
		});

		SPUriEntry entry = store.resolveURI("spacify:artist:1");
		check(entry.status == SPUriStatus::Fresh, "sync resolver yields a fresh entry");
		check(entry.data.value("name", "") == "Kraftwerk", "entry holds resolved data");
		check(fs::exists(store.cacheFilePath("spacify:artist:1")), "entry is persisted as JSON");

		store.resolveURI("spacify:artist:1");
		check(fetches == 1, "fresh entries are not refetched within max age");
		now += 10 * 60 * 1000;
		store.resolveURI("spacify:artist:1");
		check(fetches == 2, "entries older than max age are refetched");
	}

	// A new session renders the cached entity immediately, then revalidates.
	{
		SPUriStore store;
		store.setClock([&] { return now; });
		store.setPersistenceDirectory(dir.u8string());
		store.setScheduler([&](std::function<void()> task) { deferred.push_back(std::move(task)); });
		store.registerResolver("spacify:artist:", [&](const std::string &, auto resolve, auto) {
			pending.push_back(resolve);
		});

		std::vector<std::string> seen;
		auto subscription = store.subscribe("spacify:artist:1", [&](const std::string &, const SPUriEntry &e) {
			seen.push_back(space::toString(e.status));
		});

		SPUriEntry entry = store.resolveURI("spacify:artist:1");
		check(entry.status == SPUriStatus::Loading, "cached entry is revalidated");
		check(entry.data.value("name", "") == "Kraftwerk", "cached data is available while loading");
		check(pending.size() == 1, "one fetch is in flight");

		store.resolveURI("spacify:artist:1");
		check(pending.size() == 1, "no duplicate fetch while loading");

		pending[0]({{"name", "Kraftwerk (updated)"}});
		check(store.get("spacify:artist:1").status == SPUriStatus::Loading,
			  "results wait for the scheduler");
		for (auto &task : deferred)
			task();
		deferred.clear();

		entry = store.get("spacify:artist:1");
		check(entry.status == SPUriStatus::Fresh, "scheduled result is applied");
		check(entry.data.value("name", "") == "Kraftwerk (updated)", "fresh data replaces cached data");
		check(seen == std::vector<std::string>({"cached", "loading", "fresh"}),
			  "listener sees cached -> loading -> fresh");
	}

	// Superseded responses are ignored; failures keep the last known data.
	{
		SPUriEntry state;
		state = SPUriStore::reduce(state, {space::SPUriAction::Type::Pending, "u", nullptr, "", 1});
		state = SPUriStore::reduce(state, {space::SPUriAction::Type::Pending, "u", nullptr, "", 2});
		state = SPUriStore::reduce(state, {space::SPUriAction::Type::Fulfilled, "u", {{"v", 1}}, "", 1});
		check(state.status == SPUriStatus::Loading && state.data.is_null(), "stale response is ignored");
		state = SPUriStore::reduce(state, {space::SPUriAction::Type::Fulfilled, "u", {{"v", 2}}, "", 2});
		state = SPUriStore::reduce(state, {space::SPUriAction::Type::Pending, "u", nullptr, "", 3});
		state = SPUriStore::reduce(state, {space::SPUriAction::Type::Rejected, "u", nullptr, "offline", 3});
		check(state.status == SPUriStatus::Error && state.data.value("v", 0) == 2,
			  "rejection keeps last known data");
		state = SPUriStore::reduce(state, {space::SPUriAction::Type::Patch, "u", {{"w", 3}}});
		check(state.data.value("v", 0) == 2 && state.data.value("w", 0) == 3, "patch merges data");
	}

	// Listeners may unsubscribe each other during a notification.
	{
		SPUriStore store;
		SPUriStore::Subscription second;
		int secondCalls = 0;
		auto first = store.subscribe("", [&](const std::string &, const SPUriEntry &) { second.reset(); });
		second = store.subscribe("", [&](const std::string &, const SPUriEntry &) { ++secondCalls; });
		store.dispatch({space::SPUriAction::Type::Patch, "spacify:track:1", {{"name", "x"}}});
		check(secondCalls == 0 && !second.active(), "unsubscribed listener is skipped");
	}

	// Evicting removes the cache file.
	{
		SPUriStore store;
		store.setPersistenceDirectory(dir.u8string());
		store.get("spacify:artist:1");
		store.dispatch({space::SPUriAction::Type::Evict, "spacify:artist:1"});
		check(!fs::exists(store.cacheFilePath("spacify:artist:1")), "evict deletes the cache file");
	}

	fs::remove_all(dir);
	return failures == 0 ? 0 : 1;
}
