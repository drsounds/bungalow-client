#include "EntityResolvers.h"

#include "Database.h"
#include "SPUriStore.h"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace space {

namespace {

const std::string kArtistPrefix = "bungalow:artist:";
const std::string kTrackPrefix = "bungalow:track:";

std::string idFromUri(const std::string& uri, const std::string& prefix) {
    std::string id = uri.substr(prefix.size());
    return id.substr(0, id.find_first_of(":#"));
}

nlohmann::json artistsToJson(const std::vector<Database::ArtistRelationship>& artists) {
    nlohmann::json out = nlohmann::json::array();
    for (const auto& artist : artists) {
        out.push_back({
            {"uri", kArtistPrefix + artist.artistIsni},
            {"isni", artist.artistIsni},
            {"name", artist.artistName},
            {"primary", artist.isPrimary},
        });
    }
    return out;
}

} // namespace

void registerEntityResolvers(SPUriStore& store, Database& database) {
    // SQLite lookups are fast enough to resolve synchronously on the UI
    // thread; slow (network) resolvers should call resolve() from a worker.
    store.registerResolver(kArtistPrefix,
        [&database](const std::string& uri, SPUriStore::ResolveCallback resolve,
                    SPUriStore::RejectCallback reject) {
            const Database::ArtistRecord artist = database.getArtist(idFromUri(uri, kArtistPrefix));
            if (artist.isni.empty()) {
                reject("Artist not found");
                return;
            }
            resolve({
                {"uri", uri},
                {"type", "artist"},
                {"isni", artist.isni},
                {"name", artist.name},
            });
        });

    store.registerResolver(kTrackPrefix,
        [&database](const std::string& uri, SPUriStore::ResolveCallback resolve,
                    SPUriStore::RejectCallback reject) {
            int trackId = 0;
            try {
                trackId = std::stoi(idFromUri(uri, kTrackPrefix));
            } catch (const std::exception&) {
                reject("Invalid track id");
                return;
            }
            const Database::TrackDetails track = database.getTrackDetails(trackId);
            if (track.recording.isrc.empty()) {
                reject("Track not found");
                return;
            }
            nlohmann::json release = nullptr;
            if (!track.release.upc.empty()) {
                release = {
                    {"uri", "bungalow:release:" + track.release.upc},
                    {"upc", track.release.upc},
                    {"name", track.release.name},
                    {"releaseDate", track.release.releaseDate},
                    {"artists", artistsToJson(track.releaseArtists)},
                };
            }
            resolve({
                {"uri", uri},
                {"type", "track"},
                {"id", track.id},
                {"name", track.recording.name},
                {"version", track.recording.version},
                {"isrc", track.recording.isrc},
                {"release", release},
                {"artists", artistsToJson(track.recordingArtists)},
            });
        });
}

} // namespace space
