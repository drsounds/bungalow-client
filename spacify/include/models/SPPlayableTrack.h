#pragma once

#include "SPModel.h"
#include <sqlite3.h>
#include <cstdint>
#include <vector>

class SPDBConnection;
class SPPlayable;
class SPTrack;

/// Maps the `playable_tracks` junction table
/// (playable_id FK → playables, track_id FK → tracks).
class SPPlayableTrack : public SPModel {
public:
    int64_t playableId = 0;
    int64_t trackId    = 0;

    SPPlayable* playable = nullptr; // non-owning; populated by caller
    SPTrack*    track    = nullptr; // non-owning; populated by caller

    SPPlayableTrack() = default;
    SPPlayableTrack(int64_t playableId, int64_t trackId);

    bool isPersisted() const override { return playableId > 0 && trackId > 0; }
    bool save(SPDBConnection& conn) override;
    bool remove(SPDBConnection& conn) override;

    static std::vector<SPPlayableTrack> findByPlayable(SPDBConnection& conn,
                                                        int64_t playableId);
    static std::vector<SPPlayableTrack> findByTrack(SPDBConnection& conn,
                                                     int64_t trackId);
    static std::vector<SPPlayableTrack> findAll(SPDBConnection& conn);

private:
    static SPPlayableTrack fromStatement(sqlite3_stmt* stmt);
};
