#pragma once

#include "PlayQueueEntry.h"
#include "datasource/EntityDataSource.h"
#include "service/AvailabilityResolver.h"

#include <memory>
#include <vector>

namespace space {

/**
 * MediaEntityDataSource — a table of entities that may be played. It answers
 * the table's playback questions (which row is playing, which are greyed
 * out) and turns its rows into a play queue.
 */
class MediaEntityDataSource : public EntityDataSource {
public:
    /** The row at @p row, or nullptr when out of range. */
    virtual std::shared_ptr<const MediaEntity> mediaAt(int row) const = 0;

    std::shared_ptr<const Entity> entityAt(int row) const final { return mediaAt(row); }

    /** One play-queue entry per row, in display order. */
    std::vector<PlayQueueEntry> buildQueueEntries() const;

    /**
     * Ask @p resolver about rows without a local file, so the ones another
     * service can play aren't greyed out. Until it answers they are.
     */
    void setAvailability(std::weak_ptr<service::AvailabilityResolver> resolver) {
        m_availability = std::move(resolver);
    }

    // ── SPTableDataSource interface ───────────────────────────────────────────
    std::string getRowUri(int index)      override;
    int         getTrackIdForRow(int row) override;
    bool        isRowPlayable(int row)    override;

private:
    /** A row without a local file that another service can play. */
    bool playableElsewhere(const MediaEntity& media) const;

    std::weak_ptr<service::AvailabilityResolver> m_availability;
};

} // namespace space
