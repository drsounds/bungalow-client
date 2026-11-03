#pragma once

#include <sqlite3.h>

#include <cstdint>
#include <string>
#include <vector>

namespace space {

/** Metadata read from an audio file's embedded tags (with filename fallback). */
struct AudioFileMetadata {
    std::string              title;        // cleaned title (version suffix removed)
    std::string              version;      // e.g. "Radio Edit"
    std::string              artists;      // artist string as written in the tag
    std::vector<std::string> artistList;   // artists split into individual names
    std::string              albumArtist;
    std::string              album;
    std::string              isrc;
    std::string              releaseDate;  // as written in the tag, e.g. "2013" or "2013-05-17"
    int                      trackNumber = 0;
    int                      duration    = 0; // milliseconds, 0 if unknown
    bool                     hasTags     = false; // title or artist came from embedded tags,
                                                  // not the "Artist - Title" filename fallback
};

/**
 * AudioImporter
 *
 * Reads embedded metadata from local audio files (ID3v1/v2, FLAC and Ogg
 * Vorbis comments, MP4/M4A atoms) and imports them into the library database:
 *
 *   files → audio_files (metadata) → playables
 *   recordings / artists / releases / tracks (matched or created; the
 *   tag's track number becomes tracks.number)
 *   track_audio_files + playable_tracks link the file to its track,
 *   and a save puts the track in the library.
 */
class AudioImporter {
public:
    struct Result {
        int64_t fileId      = -1;
        int64_t audioFileId = -1;
        int64_t playableId  = -1;
        int64_t trackId     = -1;  // -1 if the file could not be linked to a track
    };

    /** Read tags from @p path, falling back to "Artist - Title" in the filename. */
    static AudioFileMetadata readMetadata(const std::string& path);

    /** Import @p path into @p db and link it to a (possibly new) track. */
    static Result importFile(sqlite3* db, const std::string& path);

    /** Strip a version suffix ("Title (Radio Edit)") from @p title and return it. */
    static std::string extractVersion(std::string& title);
};

} // namespace space
