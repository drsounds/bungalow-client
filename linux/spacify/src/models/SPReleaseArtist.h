#pragma once

#include "SPModel.h"
#include <sqlite3.h>
#include <string>
#include <vector>

class SPDBConnection;
class SPRelease;
class SPArtist;

/// Maps the `release_artists` junction table
/// (release_id FK → releases, artist_id FK → artists,
///  number INTEGER, is_primary INTEGER).
class SPReleaseArtist : public SPModel {
public:
    std::string releaseId;
    std::string artistId;
    int         number          = 0;
    bool        isPrimaryArtist = false;

    SPRelease* release = nullptr; // non-owning; populated by caller
    SPArtist*  artist  = nullptr; // non-owning; populated by caller

    SPReleaseArtist() = default;
    SPReleaseArtist(std::string releaseId, std::string artistId,
                    int number = 0, bool isPrimary = false);

    bool isPersisted() const override { return !releaseId.empty() && !artistId.empty(); }
    bool save(SPDBConnection& conn) override;
    bool remove(SPDBConnection& conn) override;

    static std::vector<SPReleaseArtist> findByRelease(SPDBConnection& conn,
                                                       const std::string& releaseId);
    static std::vector<SPReleaseArtist> findByArtist(SPDBConnection& conn,
                                                      const std::string& artistId);
    static std::vector<SPReleaseArtist> findAll(SPDBConnection& conn);

private:
    static SPReleaseArtist fromStatement(sqlite3_stmt* stmt);
};