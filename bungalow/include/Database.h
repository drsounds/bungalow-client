#pragma once

#include <sqlite3.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <utility>

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
 *                    release_id → releases (nullable), number (position)
 *  release_artists – junction: release_id → releases, artist_id → artists,
 *                    number, is_primary
 *  saves           – id (PK autoincrement), track_id → tracks,
 *                    saved_at, deleted_at
 *  music_service_tracks – remembered "Play with…" picks: track_id → tracks
 *                    (nullable), track_key, service_id, match_isrc/title/
 *                    artist/uri (see service::ResolutionStore)
 *  bookmarks       – id (PK autoincrement), uri (UNIQUE), title,
 *                    parent_id → bookmarks, created_at
 *  playlists       – id (UUID4 TEXT PK), name, handle (federated owner,
 *                    @user@instance), created_at
 *  playlist_items  – ordered content refs for each local playlist
 *  rootlist        – id (PK autoincrement), uri (UNIQUE), title, position
 *                    (0-based, contiguous) — the user's ordered sidebar list
 *                    (bungalow:rootlist), e.g.
 *                    musik:federated:user:<handle>:playlist:<uuid>
 *  db_tables       – id (PK autoincrement), name (UNIQUE), columns, display_name,
 *                    noun_singular, noun_plural, description
 *  user_<name>     – one per user-defined table, id (UUID4 TEXT PK) + columns
 */

// ── Custom user-defined table support ────────────────────────────────────────

/**
 * Describes one user-defined table stored in the db_tables meta-table.
 * Columns are ordered pairs of (column_name, stored_type).
 * Supported stored_types:
 *   "TEXT"              — plain text
 *   "INTEGER"           — integer
 *   "REAL"              — floating-point
 *   "BELONGS_TO:<name>" — relation: stores bungalow:<name>:<uuid>, joined on display
 */
struct CustomTableDef {
    int         id           = -1;
    std::string name;         // db table name (without user_ prefix)
    std::string displayName;  // human display name, e.g. "Music Artists"
    std::string nounSingular; // e.g. "Artist"
    std::string nounPlural;   // e.g. "Artists"
    std::string description;  // purpose of this table
    std::vector<std::pair<std::string, std::string>> columns; // {col_name, col_type}
};

/** One data row from a user-defined table. id is the UUID4 TEXT primary key. */
struct CustomRow {
    std::string              id;     // UUID4
    std::vector<std::string> values; // parallel to CustomTableDef::columns
};

class Database {
public:
    struct Bookmark {
        int         id       = -1;
        std::string uri;
        std::string title;
        int         parentId = -1;  // -1 means top-level
    };

    /** One entry of the rootlist. */
    struct RootlistNode {
        int         id       = -1;
        std::string uri;
        std::string title;
        int         position = 0;
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

    /** Artist plus how much of the library credits them (for browse lists). */
    struct ArtistSummary {
        ArtistRecord artist;
        int          recordingCount = 0;
        int          releaseCount   = 0;
    };

    /** Recording plus its comma-joined artist names (for browse lists). */
    struct RecordingSummary {
        RecordingRecord recording;
        std::string     artists;
    };

    /** Release plus its comma-joined artist names and track count. */
    struct ReleaseSummary {
        ReleaseRecord release;
        std::string   artists;
        int           trackCount = 0;
    };

    /**
     * One track joined to its recording, release and recording artists —
     * the display row for every track list (all tracks, a release, an artist).
     * playUri is the first playable URI for the track, empty when unplayable.
     */
    struct TrackRow {
        int         id     = -1;
        int         number = 0;
        std::string isrc;
        std::string name;
        std::string version;
        std::string artists;
        std::string upc;
        std::string releaseName;
        std::string playUri;
        bool        saved = false;
    };

    enum class OpenMode {
        ReadWrite, ///< create the file and schema if needed (the application's connection)
        ReadOnly,  ///< an existing database, untouched; usable from a background thread
    };

    /**
     * Open (or create) the SQLite database at @p path.
     * Throws std::runtime_error if the file cannot be opened.
     */
    explicit Database(const std::string& path, OpenMode mode = OpenMode::ReadWrite);
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

    // ── Rootlist ──────────────────────────────────────────────────────────────

    /** The rootlist, ordered by position. */
    std::vector<RootlistNode> getRootlist();

    /**
     * Insert a node at @p position (clamped to the list), moving the nodes
     * from there on down by one. Returns false if @p uri is already listed.
     */
    bool insertRootlistNode(const std::string& uri, const std::string& title, int position);

    /** Set the title of the node for @p uri. */
    void renameRootlistNode(const std::string& uri, const std::string& title);

    /** Move the node for @p uri to @p position (clamped), shifting the nodes in between. */
    void moveRootlistNode(const std::string& uri, int position);

    /** Remove the node for @p uri and close the gap it leaves. */
    void removeRootlistNode(const std::string& uri);

    /** Change the URI of the node for @p uri, keeping its place. */
    void setRootlistNodeUri(const std::string& uri, const std::string& newUri);

    // ── Playlists ─────────────────────────────────────────────────────────────

    /** Create a playlist owned by the federated @p handle. Returns its UUID4. */
    std::string createPlaylist(const std::string& name, const std::string& handle);
    void setPlaylistHandle(const std::string& id, const std::string& handle);
    void renamePlaylist(const std::string& id, const std::string& name);
    void deletePlaylist(const std::string& id);
    /** The playlist's name, or "" if there is no playlist with @p id. */
    std::string getPlaylistName(const std::string& id);
    /** Content references in playlist order; false indicates a database error. */
    bool getPlaylistRefs(const std::string& id, std::vector<std::string>& refs);
    /** Insert @p refs at @p position (clamped to the playlist). */
    bool insertPlaylistRefs(const std::string& id, const std::vector<std::string>& refs, int position);
    /** Replaces the playlist's entries with @p ordered. */
    bool setPlaylistRefs(const std::string& id, const std::vector<std::string>& ordered);

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

    // ── Music browsing ────────────────────────────────────────────────────────

    /** A fresh "local:<uuid4>" key for entities created without an ISNI/ISRC/UPC. */
    static std::string generateLocalId();

    /** Single-row lookups; the key field is empty when no row matches (see also getArtist). */
    RecordingRecord getRecording(const std::string& isrc);
    ReleaseRecord   getRelease(const std::string& upc);

    /** Browse lists, ordered by name (case-insensitive). */
    std::vector<ArtistSummary>    getArtistSummaries();
    std::vector<RecordingSummary> getRecordingSummaries();
    std::vector<ReleaseSummary>   getReleaseSummaries();

    /** Credited artists, ordered by credit number. */
    std::vector<ArtistRelationship> getRecordingArtists(const std::string& isrc);
    std::vector<ArtistRelationship> getReleaseArtists(const std::string& upc);

    /** Every track in the database, ordered by release then position. */
    std::vector<TrackRow> getAllTracks();
    /** Tracks on one release, in release order (number, then insertion). */
    std::vector<TrackRow> getTracksForRelease(const std::string& upc);
    /** Tracks of every recording the artist is credited on. */
    std::vector<TrackRow> getTracksForArtist(const std::string& isni);
    /** Tracks of one recording (one per release it appears on). */
    std::vector<TrackRow> getTracksForRecording(const std::string& isrc);
    /** Tracks whose recording is named @p title (any case). */
    std::vector<TrackRow> getTracksByTitle(const std::string& title);
    /** Releases the artist is credited on, or that contain their recordings. */
    std::vector<ReleaseSummary> getReleasesForArtist(const std::string& isni);

    /** Set a track's position on its release (0 = unnumbered). */
    void setTrackNumber(int64_t trackId, int number);
    /** Move a track to another release (empty upc = no release). */
    void setTrackRelease(int64_t trackId, const std::string& releaseUpc);

    /**
     * Deletes. Junction rows (credits, saves, playables, audio-file links)
     * go with them through ON DELETE CASCADE. Deleting a release keeps its
     * tracks as release-less recordings (tracks.release_id is SET NULL).
     */
    void deleteArtist(const std::string& isni);
    void deleteRecording(const std::string& isrc);
    void deleteRelease(const std::string& upc);
    void deleteTrack(int64_t trackId);

    /** Soft-delete the active save for a track (removes it from the library). */
    void removeSave(int64_t trackId);

    // ── Custom user-defined table CRUD ────────────────────────────────────────

    /** Returns all user-defined table definitions ordered by name. */
    std::vector<CustomTableDef> getCustomTables();

    /** Returns the definition for a single table, or a default-constructed value if not found. */
    CustomTableDef getCustomTable(const std::string& name);

    /**
     * Create a new user table.
     * @p cols is a list of (column_name, stored_type) pairs.
     * Returns the new table id, or -1 on failure.
     */
    int createCustomTable(const std::string& name,
                          const std::vector<std::pair<std::string, std::string>>& cols,
                          const std::string& displayName   = "",
                          const std::string& nounSingular  = "",
                          const std::string& nounPlural    = "",
                          const std::string& description   = "");

    /** Update display-metadata for an existing table (does not touch columns). */
    void updateCustomTableMeta(const CustomTableDef& def);

    /**
     * Drop a user-defined table and remove its meta-entry.
     * No-op if the table does not exist.
     */
    void deleteCustomTable(const std::string& name);

    /** Return all rows from a user-defined table, ordered by id. */
    std::vector<CustomRow> getCustomRows(const std::string& tableName);

    /** Return a single row by UUID; CustomRow::id is empty if not found. */
    CustomRow getCustomRow(const std::string& tableName, const std::string& rowUuid);

    /**
     * Insert a new row.  @p values must be parallel to the table's columns.
     * Returns the newly generated UUID4, or "" on failure.
     */
    std::string insertCustomRow(const std::string& tableName,
                        const CustomTableDef& def,
                        const std::vector<std::string>& values);

    /**
     * Update an existing row by UUID.  @p values must be parallel to the table's columns.
     */
    void updateCustomRow(const std::string& tableName,
                         const CustomTableDef& def,
                         const std::string& rowUuid,
                         const std::vector<std::string>& values);

    /** Delete a row by UUID. */
    void deleteCustomRow(const std::string& tableName, const std::string& rowUuid);

    /** Public alias for decodeColumns — usable where only a raw sqlite3* is available. */
    static std::vector<std::pair<std::string, std::string>>
    decodeColumnsPublic(const std::string& encoded) { return decodeColumns(encoded); }

private:
    static constexpr int kBusyTimeoutMs = 3000;

    sqlite3* m_db = nullptr;

    void initialize();
    void initializeCustomTables();
    void exec(const char* sql);

    // Helpers for custom-table column encoding/decoding
    static std::string encodeColumns(const std::vector<std::pair<std::string, std::string>>& cols);
    static std::vector<std::pair<std::string, std::string>> decodeColumns(const std::string& encoded);
};
