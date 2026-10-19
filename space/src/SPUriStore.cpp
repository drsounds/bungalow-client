#include "SPUriStore.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

namespace fs = std::filesystem;

namespace space {

namespace {

// Longest cache file name before falling back to a hashed name; keeps paths
// well inside the Windows MAX_PATH limit.
constexpr size_t kMaxFileNameLength = 120;

int64_t systemNow() {
	using namespace std::chrono;
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

uint64_t fnv1a(const std::string &text) {
	uint64_t hash = 14695981039346656037ull;
	for (unsigned char c : text) {
		hash ^= c;
		hash *= 1099511628211ull;
	}
	return hash;
}

// Reversible, filesystem-safe encoding: spacify:artist:1 -> spacify%3Aartist%3A1
std::string encodeFileName(const std::string &uri) {
	static const char *hex = "0123456789ABCDEF";
	std::string out;
	for (unsigned char c : uri) {
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
			c == '-' || c == '_' || c == '.') {
			out.push_back(static_cast<char>(c));
		} else {
			out.push_back('%');
			out.push_back(hex[c >> 4]);
			out.push_back(hex[c & 0x0F]);
		}
	}
	if (out.size() > kMaxFileNameLength) {
		char suffix[17];
		std::snprintf(suffix, sizeof(suffix), "%016llx",
					  static_cast<unsigned long long>(fnv1a(uri)));
		out = out.substr(0, kMaxFileNameLength - 17) + "~" + suffix;
	}
	return out + ".json";
}

} // namespace

const char *toString(SPUriStatus status) {
	switch (status) {
	case SPUriStatus::Empty: return "empty";
	case SPUriStatus::Cached: return "cached";
	case SPUriStatus::Loading: return "loading";
	case SPUriStatus::Fresh: return "fresh";
	case SPUriStatus::Error: return "error";
	}
	return "empty";
}

bool SPUriEntry::operator==(const SPUriEntry &other) const {
	return status == other.status && updatedAt == other.updatedAt &&
		   requestId == other.requestId && error == other.error && data == other.data;
}

// ── Subscription ─────────────────────────────────────────────────────────────

struct SPUriStore::Subscription::Slot {
	std::string uri;
	Listener listener;
	bool active = true;
};

SPUriStore::Subscription::Subscription(SPUriStore *store, std::shared_ptr<Slot> slot)
	: m_store(store), m_slot(std::move(slot)) {}

SPUriStore::Subscription::Subscription(Subscription &&other) noexcept
	: m_store(other.m_store), m_slot(std::move(other.m_slot)) {
	other.m_store = nullptr;
}

SPUriStore::Subscription &SPUriStore::Subscription::operator=(Subscription &&other) noexcept {
	if (this != &other) {
		reset();
		m_store = other.m_store;
		m_slot = std::move(other.m_slot);
		other.m_store = nullptr;
	}
	return *this;
}

SPUriStore::Subscription::~Subscription() {
	reset();
}

void SPUriStore::Subscription::reset() {
	if (m_store && m_slot)
		m_store->unsubscribe(m_slot);
	m_store = nullptr;
	m_slot.reset();
}

bool SPUriStore::Subscription::active() const {
	return m_slot && m_slot->active;
}

// ── Store ────────────────────────────────────────────────────────────────────

SPUriStore &SPUriStore::instance() {
	static SPUriStore store;
	return store;
}

SPUriStore::SPUriStore()
	: m_scheduler([](std::function<void()> task) { task(); }), m_clock(systemNow) {}

SPUriEntry SPUriStore::reduce(SPUriEntry state, const SPUriAction &action) {
	switch (action.type) {
	case SPUriAction::Type::Hydrate:
		// Never let a disk read clobber data that is already in memory.
		if (state.status != SPUriStatus::Empty)
			return state;
		state.data = action.payload;
		state.updatedAt = action.timestamp;
		state.status = SPUriStatus::Cached;
		return state;

	case SPUriAction::Type::Pending:
		state.status = SPUriStatus::Loading;
		state.requestId = action.requestId;
		state.error.clear();
		return state;

	case SPUriAction::Type::Fulfilled:
		// Ignore responses from fetches that were superseded.
		if (action.requestId != state.requestId)
			return state;
		state.data = action.payload;
		state.updatedAt = action.timestamp;
		state.status = SPUriStatus::Fresh;
		state.error.clear();
		return state;

	case SPUriAction::Type::Rejected:
		if (action.requestId != state.requestId)
			return state;
		state.status = SPUriStatus::Error;
		state.error = action.error;
		return state;

	case SPUriAction::Type::Patch:
		if (state.data.is_object() && action.payload.is_object())
			state.data.merge_patch(action.payload);
		else
			state.data = action.payload;
		state.updatedAt = action.timestamp;
		if (state.status == SPUriStatus::Empty)
			state.status = SPUriStatus::Fresh;
		return state;

	case SPUriAction::Type::Invalidate:
		if (state.status == SPUriStatus::Fresh)
			state.status = SPUriStatus::Cached;
		return state;

	case SPUriAction::Type::Evict:
		return SPUriEntry{};
	}
	return state;
}

void SPUriStore::dispatch(SPUriAction action) {
	if (action.timestamp == 0)
		action.timestamp = m_clock();

	auto it = m_entries.find(action.uri);
	SPUriEntry previous = it != m_entries.end() ? it->second : SPUriEntry{};
	SPUriEntry next = reduce(previous, action);

	if (action.type == SPUriAction::Type::Evict) {
		m_entries.erase(action.uri);
		removeCacheFile(action.uri);
	} else {
		m_entries[action.uri] = next;
	}

	if (next == previous)
		return;

	const bool dataChanged = next.data != previous.data || next.updatedAt != previous.updatedAt;
	if (dataChanged && action.type != SPUriAction::Type::Hydrate &&
		action.type != SPUriAction::Type::Evict)
		persist(action.uri, next);

	notify(action.uri, next);
}

SPUriEntry SPUriStore::get(const std::string &uri) {
	if (!m_entries.count(uri))
		hydrate(uri);
	auto it = m_entries.find(uri);
	return it != m_entries.end() ? it->second : SPUriEntry{};
}

SPUriEntry SPUriStore::resolveURI(const std::string &uri, int64_t maxAgeMs) {
	SPUriEntry entry = get(uri);
	if (uri.empty() || entry.status == SPUriStatus::Loading)
		return entry;

	const int64_t maxAge = maxAgeMs < 0 ? m_defaultMaxAge : maxAgeMs;
	const bool fresh = entry.status == SPUriStatus::Fresh && m_clock() - entry.updatedAt <= maxAge;
	if (fresh)
		return entry;

	const Resolver *resolver = findResolver(uri);
	if (!resolver)
		return entry;

	const uint64_t requestId = m_nextRequestId++;
	dispatch({SPUriAction::Type::Pending, uri, nullptr, "", requestId});

	// Results may arrive on a worker thread; hop to the scheduler's thread
	// before touching the store.
	ResolveCallback resolve = [this, uri, requestId](nlohmann::json data) {
		m_scheduler([this, uri, requestId, data = std::move(data)]() mutable {
			dispatch({SPUriAction::Type::Fulfilled, uri, std::move(data), "", requestId});
		});
	};
	RejectCallback reject = [this, uri, requestId](std::string error) {
		m_scheduler([this, uri, requestId, error = std::move(error)]() mutable {
			dispatch({SPUriAction::Type::Rejected, uri, nullptr, std::move(error), requestId});
		});
	};

	try {
		(*resolver)(uri, resolve, reject);
	} catch (const std::exception &error) {
		reject(error.what());
	}
	return get(uri);
}

SPUriStore::Subscription SPUriStore::subscribe(const std::string &uri, Listener listener) {
	auto slot = std::make_shared<Subscription::Slot>();
	slot->uri = uri;
	slot->listener = std::move(listener);
	m_slots.push_back(slot);
	return Subscription(this, slot);
}

void SPUriStore::unsubscribe(const std::shared_ptr<Subscription::Slot> &slot) {
	slot->active = false;
	m_slots.erase(std::remove(m_slots.begin(), m_slots.end(), slot), m_slots.end());
}

void SPUriStore::notify(const std::string &uri, const SPUriEntry &entry) {
	// Listeners commonly re-render views, which subscribes new elements and
	// drops old ones, so iterate over a snapshot and skip slots that were
	// unsubscribed meanwhile.
	const auto snapshot = m_slots;
	for (const auto &slot : snapshot) {
		if (slot->active && (slot->uri.empty() || slot->uri == uri))
			slot->listener(uri, entry);
	}
}

void SPUriStore::registerResolver(const std::string &prefix, Resolver resolver) {
	m_resolvers[prefix] = std::move(resolver);
}

const SPUriStore::Resolver *SPUriStore::findResolver(const std::string &uri) const {
	const Resolver *best = nullptr;
	size_t bestLength = 0;
	for (const auto &[prefix, resolver] : m_resolvers) {
		if (uri.rfind(prefix, 0) == 0 && (best == nullptr || prefix.size() > bestLength)) {
			best = &resolver;
			bestLength = prefix.size();
		}
	}
	return best;
}

void SPUriStore::setPersistenceDirectory(const std::string &directory) {
	m_directory = directory;
	if (m_directory.empty())
		return;
	std::error_code error;
	fs::create_directories(fs::u8path(m_directory), error);
}

void SPUriStore::setScheduler(Scheduler scheduler) {
	m_scheduler = scheduler ? std::move(scheduler) : [](std::function<void()> task) { task(); };
}

void SPUriStore::setClock(Clock clock) {
	m_clock = clock ? std::move(clock) : Clock(systemNow);
}

void SPUriStore::reset() {
	for (const auto &slot : m_slots)
		slot->active = false;
	m_slots.clear();
	m_entries.clear();
	m_resolvers.clear();
}

std::string SPUriStore::cacheFilePath(const std::string &uri) const {
	if (m_directory.empty())
		return "";
	return (fs::u8path(m_directory) / fs::u8path(encodeFileName(uri))).u8string();
}

void SPUriStore::hydrate(const std::string &uri) {
	const std::string path = cacheFilePath(uri);
	if (path.empty())
		return;
	std::ifstream file(fs::u8path(path), std::ios::binary);
	if (!file)
		return;
	try {
		std::ostringstream source;
		source << file.rdbuf();
		nlohmann::json document = nlohmann::json::parse(source.str());
		// Hashed file names can collide; only accept the entity it was written for.
		if (!document.is_object() || document.value("uri", "") != uri || !document.contains("data"))
			return;
		SPUriAction action{SPUriAction::Type::Hydrate, uri, document.at("data")};
		action.timestamp = document.value("updatedAt", int64_t{0});
		dispatch(std::move(action));
	} catch (const std::exception &) {
		// A corrupt cache file is just a cache miss.
	}
}

void SPUriStore::persist(const std::string &uri, const SPUriEntry &entry) const {
	const std::string path = cacheFilePath(uri);
	if (path.empty() || !entry.hasData())
		return;
	nlohmann::json document = {
		{"uri", uri},
		{"updatedAt", entry.updatedAt},
		{"data", entry.data},
	};
	// Write to a temporary file and rename it so a crash never leaves a
	// half-written cache file behind.
	const fs::path target = fs::u8path(path);
	fs::path temporary = target;
	temporary += ".tmp";
	{
		std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
		if (!file)
			return;
		file << document.dump();
		if (!file)
			return;
	}
	std::error_code error;
	fs::rename(temporary, target, error);
	if (error)
		fs::remove(temporary, error);
}

void SPUriStore::removeCacheFile(const std::string &uri) const {
	const std::string path = cacheFilePath(uri);
	if (path.empty())
		return;
	std::error_code error;
	fs::remove(fs::u8path(path), error);
}

} // namespace space
