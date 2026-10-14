#pragma once

#include "SPModel.h"
#include <sqlite3.h>
#include <cstdint>
#include <vector>

class SPDBConnection;
class SPTrack;
class SPAudioFile;

/// Maps the `track_audio_files` junction table
/// (track_id FK → tracks, audio_file_id FK → audio_files).
class SPTrackAudioFile : public SPModel {
public:
    int64_t trackId     = 0;
    int64_t audioFileId = 0;

    SPTrack*     track     = nullptr; // non-owning; populated by caller
    SPAudioFile* audioFile = nullptr; // non-owning; populated by caller

    SPTrackAudioFile() = default;
    SPTrackAudioFile(int64_t trackId, int64_t audioFileId);

    bool isPersisted() const override { return trackId > 0 && audioFileId > 0; }
    bool save(SPDBConnection& conn) override;
    bool remove(SPDBConnection& conn) override;

    static std::vector<SPTrackAudioFile> findByTrack(SPDBConnection& conn, int64_t trackId);
    static std::vector<SPTrackAudioFile> findByAudioFile(SPDBConnection& conn, int64_t audioFileId);
    static std::vector<SPTrackAudioFile> findAll(SPDBConnection& conn);

private:
    static SPTrackAudioFile fromStatement(sqlite3_stmt* stmt);
};
