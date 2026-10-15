#pragma once

#include "SPModel.h"
#include <sqlite3.h>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class SPDBConnection;
class SPFileRow;

/// Maps the `audio_files` table
/// (id INTEGER PK AUTOINCREMENT, file_id FK → files,
///  name TEXT, version TEXT, artists TEXT, album TEXT,
///  duration INTEGER, cover_art TEXT).
class SPAudioFile : public SPModel {
public:
    int64_t     id       = 0;
    int64_t     fileId   = 0;
    std::string name;
    std::string version;
    std::string artists;
    std::string album;
    int64_t     duration = 0; // milliseconds
    std::string coverArt;

    SPFileRow* file = nullptr; // non-owning; populated by caller

    SPAudioFile() = default;
    SPAudioFile(int64_t fileId, std::string name, std::string version = {},
                std::string artists = {}, std::string album = {},
                int64_t duration = 0, std::string coverArt = {});

    bool isPersisted() const override { return id > 0; }
    bool save(SPDBConnection& conn) override;
    bool remove(SPDBConnection& conn) override;

    static std::optional<SPAudioFile> find(SPDBConnection& conn, int64_t id);
    static std::vector<SPAudioFile>   findAll(SPDBConnection& conn);
    static std::vector<SPAudioFile>   findByFile(SPDBConnection& conn, int64_t fileId);

private:
    static SPAudioFile fromStatement(sqlite3_stmt* stmt);
};