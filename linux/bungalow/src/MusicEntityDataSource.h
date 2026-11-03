#pragma once

#include "Database.h"
#include "PlayQueueEntry.h"
#include "../../space/src/SPTableDataSource.h"

#include <string>
#include <vector>

namespace space {

/** One display row of a music browse table, plus the keys its actions need. */
struct MusicEntityRow {
    std::vector<std::string> cells;   // parallel to the data source's columns
    std::string              key;     // ISNI / ISRC / UPC of the row's entity
    std::string              title;   // display title for the play queue
    int                      trackId = 0;
    std::string              playUri; // empty when the row has nothing to play
    std::string              artist;  // display artist for the play queue
    bool                     playable = true;
    // Track rows only:
    std::string              releaseKey; // UPC of the track's release, "" if none
    int                      number = 0;
    bool                     saved  = false;
};

/**
 * MusicEntityDataSource
 *
 * A fixed-column table data source whose rows are filled by the owning view
 * (artists, recordings, releases, tracks). Views rebuild the rows from
 * Database queries on navigate() and after every mutation.
 */
class MusicEntityDataSource : public SPTableDataSource {
public:
    explicit MusicEntityDataSource(std::vector<std::string> columns);
    ~MusicEntityDataSource() override = default;

    void setRows(std::vector<MusicEntityRow> rows) { m_rows = std::move(rows); }

    /** The row at @p index, or nullptr when out of range. */
    const MusicEntityRow* rowAt(int index) const;

    /** Rows for a list of tracks: the columns are chosen by @p cells. */
    template <typename CellsFn>
    void setTrackRows(const std::vector<Database::TrackRow>& tracks, CellsFn cells) {
        std::vector<MusicEntityRow> rows;
        rows.reserve(tracks.size());
        for (const auto& t : tracks) {
            MusicEntityRow r;
            r.cells    = cells(t);
            r.key      = t.isrc;
            r.title    = t.name;
            r.trackId  = t.id;
            r.playUri  = t.playUri;
            r.artist   = t.artists;
            r.playable = !t.playUri.empty();
            r.releaseKey = t.upc;
            r.number     = t.number;
            r.saved      = t.saved;
            rows.push_back(std::move(r));
        }
        setRows(std::move(rows));
    }

    /** One play-queue entry per row, in display order. */
    std::vector<PlayQueueEntry> buildQueueEntries() const;

    // ── SPTableDataSource interface ───────────────────────────────────────────
    int         getColumnCount()                    override { return static_cast<int>(m_columns.size()); }
    int         getRowCount()                       override { return static_cast<int>(m_rows.size()); }
    std::string getTextForColumn(int column)        override;
    std::string getTextForCell(int column, int row) override;
    std::string getRowUri(int index)                override;
    int         getTrackIdForRow(int row)           override;
    bool        isRowPlayable(int row)              override;

private:
    std::vector<std::string>    m_columns;
    std::vector<MusicEntityRow> m_rows;
};

} // namespace space
