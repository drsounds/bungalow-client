#pragma once

#include "SPModel.h"
#include <sqlite3.h>
#include <string>
#include <vector>

class SPDBConnection;
class SPRecording;
class SPArtist;

/// Maps the `recording_artists` junction table
/// (recording_id FK → recordings, artist_id FK → artists,
///  is_primary INTEGER, number INTEGER).
class SPRecordingArtist : public SPModel {
public:
    std::string recordingId;
    std::string artistId;
    bool        isPrimaryArtist = false;
    int         number          = 0;

    SPRecording* recording = nullptr; // non-owning; populated by caller
    SPArtist*    artist    = nullptr; // non-owning; populated by caller

    SPRecordingArtist() = default;
    SPRecordingArtist(std::string recordingId, std::string artistId,
                      bool isPrimary = false, int number = 0);

    bool isPersisted() const override { return !recordingId.empty() && !artistId.empty(); }
    bool save(SPDBConnection& conn) override;
    bool remove(SPDBConnection& conn) override;

    static std::vector<SPRecordingArtist> findByRecording(SPDBConnection& conn,
                                                           const std::string& recordingId);
    static std::vector<SPRecordingArtist> findByArtist(SPDBConnection& conn,
                                                        const std::string& artistId);
    static std::vector<SPRecordingArtist> findAll(SPDBConnection& conn);

private:
    static SPRecordingArtist fromStatement(sqlite3_stmt* stmt);
};