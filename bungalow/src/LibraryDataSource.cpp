#include "LibraryDataSource.h"

#include "SPRichText.h"

#include <unordered_map>

namespace space {

// ── Helpers ───────────────────────────────────────────────────────────────────

static const char* kColumnNames[] = { "Name", "Artists", "Release" };

// Joins saves → tracks → recordings → releases → recording_artists → artists.
// GROUP_CONCAT aggregates artist names per save row.
static const char* kQuery = R"(
    SELECT
        rec.name                                     AS name,
        COALESCE(GROUP_CONCAT(DISTINCT a.name), '')  AS artists,
        COALESCE(rel.name, '')                       AS release_name,
        t.id                                         AS track_id,
        CASE WHEN EXISTS (
            SELECT 1 FROM playable_tracks pt WHERE pt.track_id = t.id
        ) THEN 1 ELSE 0 END                          AS is_playable,
        COALESCE((
            SELECT GROUP_CONCAT(a2.isni || char(31) || a2.name, char(30))
            FROM recording_artists ra2
            JOIN artists a2 ON a2.isni = ra2.artist_id
            WHERE ra2.recording_id = rec.isrc
        ), '')                                       AS artist_links,
        COALESCE(rel.upc, '')                        AS release_upc
    FROM saves s
    JOIN tracks             t   ON t.id            = s.track_id
    JOIN recordings         rec ON rec.isrc         = t.recording_id
    LEFT JOIN releases      rel ON rel.upc          = t.release_id
    LEFT JOIN recording_artists ra ON ra.recording_id = rec.isrc
    LEFT JOIN artists           a  ON a.isni          = ra.artist_id
    WHERE s.deleted_at IS NULL
    GROUP BY s.id
    ORDER BY s.saved_at DESC
)";

// ── Construction ──────────────────────────────────────────────────────────────

LibraryDataSource::LibraryDataSource(sqlite3* db) {
    if (db) load(db);
}

// ── SPTableDataSource interface ───────────────────────────────────────────────

int LibraryDataSource::getRowCount() {
    return static_cast<int>(m_rows.size());
}

std::string LibraryDataSource::getTextForColumn(int column) {
    if (column >= 0 && column < 3) return kColumnNames[column];
    return "";
}

std::string LibraryDataSource::getRowUri(int index) {
    if (index < 0 || index >= static_cast<int>(m_rows.size())) return "";
    return "bungalow:track:" + std::to_string(m_rows[index].trackId);
}

std::string LibraryDataSource::getTextForCell(int column, int row) {
    if (row < 0 || row >= static_cast<int>(m_rows.size())) return "";
    const auto& r = m_rows[row];
    switch (column) {
        case 0: return r.name;
        case 1: return r.artists;
        case 2: return r.releaseName;
        default: return "";
    }
}

std::string LibraryDataSource::getMarkupForCell(int column, int row) {
    if (row < 0 || row >= static_cast<int>(m_rows.size())) return "";
    switch (column) {
        case 1: return m_rows[row].artistsMarkup;
        case 2: return m_rows[row].releaseMarkup;
        default: return "";
    }
}

int LibraryDataSource::getTrackIdForRow(int row) {
    if (row < 0 || row >= static_cast<int>(m_rows.size())) return 0;
    return m_rows[row].trackId;
}

bool LibraryDataSource::isRowPlayable(int row) {
    if (row < 0 || row >= static_cast<int>(m_rows.size())) return false;
    return m_rows[row].playable;
}

// ── Data loading ──────────────────────────────────────────────────────────────

void LibraryDataSource::reload(sqlite3* db) {
    m_rows.clear();
    if (db) load(db);
}

void LibraryDataSource::load(sqlite3* db) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, kQuery, -1, &stmt, nullptr) != SQLITE_OK) return;

    auto col = [&](int i) -> std::string {
        const unsigned char* txt = sqlite3_column_text(stmt, i);
        return txt ? reinterpret_cast<const char*>(txt) : "";
    };

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        LibraryRow r;
        r.name        = col(0);
        r.artists     = col(1);
        r.releaseName = col(2);
        r.trackId     = sqlite3_column_int(stmt, 3);
        r.playable    = sqlite3_column_int(stmt, 4) != 0;

        // artist_links is "isni<US>name" per artist, separated by <RS>.
        const std::string artistLinks = col(5);
        for (size_t start = 0; start < artistLinks.size();) {
            size_t end = artistLinks.find('\x1e', start);
            if (end == std::string::npos) end = artistLinks.size();
            const std::string item = artistLinks.substr(start, end - start);
            const size_t split = item.find('\x1f');
            if (split != std::string::npos && split > 0) {
                if (!r.artistsMarkup.empty()) r.artistsMarkup += ", ";
                r.artistsMarkup += SPRichText::link(
                    "bungalow:library:artist:" + item.substr(0, split), item.substr(split + 1));
            }
            start = end + 1;
        }
        const std::string upc = col(6);
        if (!upc.empty() && !r.releaseName.empty())
            r.releaseMarkup = SPRichText::link("bungalow:library:release:" + upc, r.releaseName);
        m_rows.push_back(std::move(r));
    }

    sqlite3_finalize(stmt);
}

std::vector<PlayQueueEntry> LibraryDataSource::buildQueueEntries(sqlite3* db) const {
    // Single query: fetch first URI for every track that has a playable_tracks row.
    static const char* kUriQuery = R"(
        SELECT pt.track_id, p.uri
        FROM playable_tracks pt
        JOIN playables p ON p.id = pt.playable_id
        GROUP BY pt.track_id
    )";

    std::unordered_map<int, std::string> uriMap;
    if (db) {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, kUriQuery, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                int         tid = sqlite3_column_int(stmt, 0);
                const char* uri = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                if (uri) uriMap[tid] = uri;
            }
            sqlite3_finalize(stmt);
        }
    }

    std::vector<PlayQueueEntry> entries;
    entries.reserve(m_rows.size());
    for (const auto& row : m_rows) {
        PlayQueueEntry e;
        e.displayTitle  = row.name;
        e.displayArtist = row.artists;
        e.playable      = row.playable;
        e.trackId       = row.trackId;
        if (row.playable) {
            auto it = uriMap.find(row.trackId);
            if (it != uriMap.end()) {
                e.uri = it->second;
            } else {
                e.playable = false; // flag said playable but no URI found
            }
        }
        entries.push_back(std::move(e));
    }
    return entries;
}

} // namespace space
