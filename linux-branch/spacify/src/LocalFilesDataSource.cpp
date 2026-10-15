#include "LocalFilesDataSource.h"

#include <cstdio>

namespace space {

static const char* kColumnNames[] = { "Name", "Version", "Artists", "Album", "Duration" };

static const char* kQuery = R"(
    SELECT
        af.name,
        COALESCE(af.version, ''),
        COALESCE(af.artists, ''),
        COALESCE(af.album,   ''),
        af.duration,
        f.path
    FROM audio_files af
    JOIN files f ON f.id = af.file_id
    ORDER BY af.artists, af.name
)";

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string formatDuration(int ms) {
    if (ms <= 0) return "";
    int total = ms / 1000;
    int m = total / 60;
    int s = total % 60;
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d:%02d", m, s);
    return buf;
}

// ── Construction ──────────────────────────────────────────────────────────────

LocalFilesDataSource::LocalFilesDataSource(sqlite3* db) {
    if (db) load(db);
}

// ── SPTableDataSource interface ───────────────────────────────────────────────

int LocalFilesDataSource::getRowCount() {
    return static_cast<int>(m_rows.size());
}

std::string LocalFilesDataSource::getTextForColumn(int column) {
    if (column >= 0 && column < 5) return kColumnNames[column];
    return "";
}

std::string LocalFilesDataSource::getTextForCell(int column, int row) {
    if (row < 0 || row >= static_cast<int>(m_rows.size())) return "";
    const auto& r = m_rows[row];
    switch (column) {
        case 0: return r.name;
        case 1: return r.version;
        case 2: return r.artists;
        case 3: return r.album;
        case 4: return r.duration;
        default: return "";
    }
}

// ── Data loading ──────────────────────────────────────────────────────────────

void LocalFilesDataSource::reload(sqlite3* db) {
    m_rows.clear();
    if (db) load(db);
}

void LocalFilesDataSource::load(sqlite3* db) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, kQuery, -1, &stmt, nullptr) != SQLITE_OK) return;

    auto col = [&](int i) -> std::string {
        const unsigned char* txt = sqlite3_column_text(stmt, i);
        return txt ? reinterpret_cast<const char*>(txt) : "";
    };

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        LocalFileRow r;
        r.name     = col(0);
        r.version  = col(1);
        r.artists  = col(2);
        r.album    = col(3);
        r.duration = formatDuration(sqlite3_column_int(stmt, 4));
        r.path     = col(5);
        m_rows.push_back(std::move(r));
    }

    sqlite3_finalize(stmt);
}

std::string LocalFilesDataSource::getFilePathForRow(int row) const {
    if (row >= 0 && row < static_cast<int>(m_rows.size())) {
        return m_rows[row].path;
    }
    return "";
}

std::vector<PlayQueueEntry> LocalFilesDataSource::buildQueueEntries() const {
    std::vector<PlayQueueEntry> entries;
    entries.reserve(m_rows.size());
    for (const auto& row : m_rows) {
        PlayQueueEntry e;
        e.displayTitle  = row.name;
        e.displayArtist = row.artists;
        e.uri           = "file://" + row.path;
        e.playable      = !row.path.empty();
        entries.push_back(std::move(e));
    }
    return entries;
}

} // namespace space
