#ifndef SP_URI_STORE_H
#define SP_URI_STORE_H

#include <nlohmann/json.hpp>

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace space {

/**
 * Lifecycle of a URI entity in the store.
 *
 *  Empty    nothing known yet
 *  Cached   data hydrated from the on-disk JSON cache, not yet revalidated
 *  Loading  a resolver is fetching; `data` still holds the last known value
 *  Fresh    data fetched during this session
 *  Error    the last fetch failed; `data` still holds the last known value
 */
enum class SPUriStatus { Empty, Cached, Loading, Fresh, Error };

const char *toString(SPUriStatus status);

struct SPUriEntry {
	nlohmann::json data;          // null until something is known
	SPUriStatus status = SPUriStatus::Empty;
	int64_t updatedAt = 0;        // ms since epoch of the data in `data`
	uint64_t requestId = 0;       // id of the fetch in flight / last fetch
	std::string error;

	bool hasData() const { return !data.is_null(); }
	bool operator==(const SPUriEntry &other) const;
	bool operator!=(const SPUriEntry &other) const { return !(*this == other); }
};

struct SPUriAction {
	enum class Type {
		Hydrate,    // payload loaded from disk
		Pending,    // fetch started (requestId)
		Fulfilled,  // fetch finished (requestId, payload)
		Rejected,   // fetch failed (requestId, error)
		Patch,      // merge payload into the current data (optimistic update)
		Invalidate, // mark stale so the next resolveURI refetches
		Evict       // forget the entity, including its cache file
	};
	Type type;
	std::string uri;
	nlohmann::json payload;
	std::string error;
	uint64_t requestId = 0;
	int64_t timestamp = 0;
};

/**
 * Redux-lite global store for URI entities (mostly bungalow: URIs).
 *
 * State is a map of URI -> SPUriEntry, changed only through dispatch() and
 * the pure reduce() function. Entities are persisted as JSON files, so a
 * view can render the last known data immediately on startup while
 * resolveURI() revalidates it in the background (stale-while-revalidate).
 *
 * The store is not thread-safe: use it from the UI thread. Resolvers may
 * complete on any thread; their results are handed to the scheduler, which
 * the application points at its UI thread.
 */
class SPUriStore {
public:
	using Listener = std::function<void(const std::string &uri, const SPUriEntry &entry)>;
	using ResolveCallback = std::function<void(nlohmann::json data)>;
	using RejectCallback = std::function<void(std::string error)>;
	using Resolver = std::function<void(const std::string &uri, ResolveCallback resolve, RejectCallback reject)>;
	using Scheduler = std::function<void(std::function<void()> task)>;
	using Clock = std::function<int64_t()>;

	/// Unsubscribes when destroyed or reset. Move-only.
	class Subscription {
	public:
		Subscription() = default;
		Subscription(Subscription &&other) noexcept;
		Subscription &operator=(Subscription &&other) noexcept;
		Subscription(const Subscription &) = delete;
		Subscription &operator=(const Subscription &) = delete;
		~Subscription();
		void reset();
		bool active() const;

	private:
		friend class SPUriStore;
		struct Slot;
		Subscription(SPUriStore *store, std::shared_ptr<Slot> slot);
		SPUriStore *m_store = nullptr;
		std::shared_ptr<Slot> m_slot;
	};

	static SPUriStore &instance();

	SPUriStore();
	SPUriStore(const SPUriStore &) = delete;
	SPUriStore &operator=(const SPUriStore &) = delete;

	/// Pure reducer: next state of one entry for an action addressed to it.
	static SPUriEntry reduce(SPUriEntry state, const SPUriAction &action);

	void dispatch(SPUriAction action);

	/// Current entry, hydrating it from the JSON cache on first access.
	SPUriEntry get(const std::string &uri);

	/**
	 * Returns what is known about @p uri right now (possibly cached or empty)
	 * and starts a fetch through the matching resolver when the entry is
	 * missing, hydrated from disk, invalidated, failed, or older than
	 * @p maxAgeMs (negative: use the default max age).
	 */
	SPUriEntry resolveURI(const std::string &uri, int64_t maxAgeMs = -1);

	/// Listen to changes of one URI, or of every URI when @p uri is empty.
	[[nodiscard]] Subscription subscribe(const std::string &uri, Listener listener);

	/// Register a resolver for URIs starting with @p prefix; longest prefix wins.
	void registerResolver(const std::string &prefix, Resolver resolver);

	/// Directory for the JSON cache; empty disables persistence.
	void setPersistenceDirectory(const std::string &directory);
	const std::string &getPersistenceDirectory() const { return m_directory; }
	void setScheduler(Scheduler scheduler);
	void setClock(Clock clock);
	void setDefaultMaxAge(int64_t maxAgeMs) { m_defaultMaxAge = maxAgeMs; }

	/// Drop all in-memory state, listeners and resolvers (cache files stay).
	void reset();

	/// Path of the cache file for @p uri inside the persistence directory.
	std::string cacheFilePath(const std::string &uri) const;

private:
	std::map<std::string, SPUriEntry> m_entries;
	std::map<std::string, Resolver> m_resolvers;
	std::vector<std::shared_ptr<Subscription::Slot>> m_slots;
	std::string m_directory;
	Scheduler m_scheduler;
	Clock m_clock;
	int64_t m_defaultMaxAge = 5 * 60 * 1000;
	uint64_t m_nextRequestId = 1;

	const Resolver *findResolver(const std::string &uri) const;
	void hydrate(const std::string &uri);
	void persist(const std::string &uri, const SPUriEntry &entry) const;
	void removeCacheFile(const std::string &uri) const;
	void notify(const std::string &uri, const SPUriEntry &entry);
	void unsubscribe(const std::shared_ptr<Subscription::Slot> &slot);
};

} // namespace space

#endif // SP_URI_STORE_H
