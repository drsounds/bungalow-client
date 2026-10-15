#pragma once

#include "SPModel.h"
#include <sqlite3.h>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class SPDBConnection;
class SPRecording;
class SPRelease;

/// Maps the `tracks` table
/// (id INTEGER PK AUTOINCREMENT, recording_id FK → recordings,
///  release_id FK → releases).
class SPTrack : public SPModel {
public:
    int64_t     id          = 0;
    std::string recordingId;
    std::string releaseId;

    SPRecording* recording = nullptr; // non-owning; populated by caller
    SPRelease*   release   = nullptr; // non-owning; populated by caller

    SPTrack() = default;
    SPTrack(std::string recordingId, std::string releaseId);
    SPTrack(int64_t id, std::string recordingId, std::string releaseId);

    bool isPersisted() const override { return id > 0; }
    bool save(SPDBConnection& conn) override;
    bool remove(SPDBConnection& conn) override;

    static std::optional<SPTrack> find(SPDBConnection& conn, int64_t id);
    static std::vector<SPTrack>   findAll(SPDBConnection& conn);
    static std::vector<SPTrack>   findByRecording(SPDBConnection& conn,
                                                   const std::string& recordingId);
    static std::vector<SPTrack>   findByRelease(SPDBConnection& conn,
                                                 const std::string& releaseId);

private:
    static SPTrack fromStatement(sqlite3_stmt* stmt);
};