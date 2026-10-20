#pragma once

#include <sqlite3.h>
#include <string>
#include <vector>
#include <stdexcept>

/**
 * Database — thin wrapper around sqlite3 that owns the connection and
 * creates the full application schema on first open.
 *
 * Schema overview
 * ───────────────
 *  recordings      – ISRC (PK), name, version
 *  artists         – ISNI (PK), name
 *  recording_artists – junction: recording_id → recordings, artist_id → artists,
 *                      is_primary, number
 *  labels          – id (PK autoincrement), name
 *  releases        – upc (PK), name, release_date, label_id → labels, cover_art
 *  tracks          – id (PK autoincrement), recording_id → recordings,
 *                    release_id → releases (nullable)
 *  release_artists – junction: release_id → releases, artist_id → artists,
 *                    number, is_primary
 *  saves           – id (PK autoincrement), track_id → tracks,
 *                    saved_at, deleted_at
 *  bookmarks       – id (PK autoincrement), uri (UNIQUE), title,
 *                    parent_id → bookmarks, created_at
 */
class Database {
public:
    struct Bookmark {
        int         id       = -1;
        std::string uri;
        std::string title;
        int         parentId = -1;  // -1 means top-level
    };

    // ── Music metadata records ────────────────────────────────────────────────

    struct ArtistRecord {
        std::string isni;
        std::string name;
    };

    struct RecordingRecord {
        std::string isrc;
        std::string name;
        std::string version;
    };

    struct ReleaseRecord {
        std::string upc;
        std::string name;
        std::string releaseDate;
    };

    struct ArtistRelationship {
        std::string artistIsni;
        std::string artistName;
        bool        isPrimary = false;
        int         number    = 0;
    };

    struct TrackDetails {
        int                              id = -1;
        RecordingRecord                  recording;
        ReleaseRecord                    release;       // empty upc = no release
        std::vector<ArtistRelationship>  recordingArtists;
        std::vector<ArtistRelationship>  releaseArtists;
    };

    /**
     * Open (or create) the SQLite database at @p path.
     * Throws std::runtime_error if the file cannot be opened.
     */
    explicit Database(const std::string& path);
    ~Database();

    // Non-copyable, movable
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    Database(Database&&) noexcept;
    Database& operator=(Database&&) noexcept;

    /** Returns the raw sqlite3 handle for use with sqlite3_prepare_v2 etc. */
    sqlite3* handle() const { return m_db; }

    // ── Bookmark CRUD ─────────────────────────────────────────────────────────

    /** Add a bookmark. parentId=-1 for top-level entries. No-op if uri already exists. */
    void addBookmark(const std::string& uri, const std::string& title, int parentId = -1);

    /** Remove a bookmark by URI. Children are cascade-deleted by the schema. */
    void removeBookmark(const std::string& uri);

    /** Returns true if a bookmark for uri exists. */
    bool isBookmarked(const std::string& uri);

    // ── Accepted third-party domains ──────────────────────────────────────────

    /** Returns true if the user has previously accepted the third-party warning for hostname. */
    bool isDomainAccepted(const std::string& hostname);

    /** Record that the user accepted the third-party warning for hostname. No-op if already present. */
    void addAcceptedDomain(const std::string& hostname);

    /** Returns the id of a bookmark, or -1 if not found. */
    int getBookmarkId(const std::string& uri);

    /** Top-level bookmarks whose URI starts with uriPrefix, ordered by created_at. */
    std::vector<Bookmark> getTopLevelBookmarks(const std::string& uriPrefix);

    /** All child bookmarks of the given parent, ordered by created_at. */
    std::vector<Bookmark> getChildBookmarks(int parentId);

    // ── Music metadata CRUD ───────────────────────────────────────────────────

    /** Returns all artists ordered by name. */
    std::vector<ArtistRecord>    getAllArtists();
    /** Returns the artist with @p isni, or an empty isni if not found. */
    ArtistRecord                 getArtist(const std::string& isni);
    /** Returns all recordings ordered by name. */
    std::vector<RecordingRecord> getAllRecordings();
    /** Returns all releases ordered by name. */
    std::vector<ReleaseRecord>   getAllReleases();

    /** Insert or update an artist row. */
    void upsertArtist(const std::string& isni, const std::string& name);
    /** Insert or update a recording row. */
    void upsertRecording(const std::string& isrc, const std::string& name, const std::string& version);
    /** Insert or update a release row. */
    void upsertRelease(const std::string& upc, const std::string& name, const std::string& releaseDate);

    /**
     * Insert or find a track for the given recording / release combination.
     * @p releaseUpc may be empty for recordings without a release.
     * Returns the track id.
     */
    int64_t upsertTrack(const std::string& recordingIsrc, const std::string& releaseUpc);

    /** Replace all recording_artists rows for @p recordingIsrc. */
    void setRecordingArtists(const std::string& recordingIsrc, const std::vector<ArtistRelationship>& artists);
    /** Replace all release_artists rows for @p releaseUpc. */
    void setReleaseArtists(const std::string& releaseUpc, const std::vector<ArtistRelationship>& artists);

    /**
     * Add a save for @p trackId.
     * If a non-deleted save already exists, it is left as-is and its id is returned.
     * If a soft-deleted save exists, it is reactivated (deleted_at set to NULL).
     * Otherwise a new save row is inserted.
     * Returns the save id.
     */
    int64_t ensureSave(int64_t trackId);

    /** Load full details for a track (recording, release, artist relationships). */
    TrackDetails getTrackDetails(int trackId);

private:
    sqlite3* m_db = nullptr;

    void initialize();
    void exec(const char* sql);
};
