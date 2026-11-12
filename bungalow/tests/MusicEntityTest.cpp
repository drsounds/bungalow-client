// The entities MusicEntityRepository loads through the ORM models must list
// the same rows, in the same order, as the Database's joined SQL queries.

#include "Database.h"
#include "app/library/MusicEntityRepository.h"
#include "models/SPDBConnection.h"
#include "models/SPPlayable.h"
#include "models/SPPlayableTrack.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

using space::MusicColumn;
using space::MusicEntityRepository;

namespace {

int failures = 0;

void expect(const std::string& actual, const std::string& expected, const std::string& what) {
    if (actual == expected) return;
    std::cerr << what << ":\n  expected \"" << expected << "\"\n  got      \"" << actual << "\"\n";
    ++failures;
}

std::string describe(const std::vector<Database::TrackRow>& rows) {
    std::string out;
    for (const auto& t : rows)
        out += std::to_string(t.id) + "|" + (t.number > 0 ? std::to_string(t.number) : "") + "|" +
               t.isrc + "|" + t.name + "|" + t.version + "|" + t.artists + "|" + t.upc + "|" +
               t.releaseName + "|" + t.playUri + "|" + (t.saved ? "Yes" : "") + "\n";
    return out;
}

std::string describe(const MusicEntityRepository::Tracks& tracks) {
    std::string out;
    for (const auto& t : tracks)
        out += t->id() + "|" + t->text(MusicColumn::Number) + "|" + t->isrc() + "|" +
               t->text(MusicColumn::Name) + "|" + t->text(MusicColumn::Version) + "|" +
               t->text(MusicColumn::Artists) + "|" + (t->release() ? t->release()->upc : "") + "|" +
               t->text(MusicColumn::Release) + "|" + t->playUri() + "|" +
               t->text(MusicColumn::Saved) + "\n";
    return out;
}

std::string describe(const std::vector<Database::ReleaseSummary>& rows) {
    std::string out;
    for (const auto& r : rows)
        out += r.release.upc + "|" + r.release.name + "|" + r.release.releaseDate + "|" +
               r.artists + "|" + std::to_string(r.trackCount) + "\n";
    return out;
}

std::string describe(const MusicEntityRepository::Releases& releases) {
    std::string out;
    for (const auto& r : releases)
        out += r->id() + "|" + r->text(MusicColumn::Name) + "|" + r->text(MusicColumn::Date) + "|" +
               r->text(MusicColumn::Artists) + "|" + r->text(MusicColumn::TrackCount) + "\n";
    return out;
}

void addPlayable(Database& db, int64_t trackId, const std::string& uri) {
    SPDBConnection conn(db);
    SPPlayable playable(uri);
    playable.save(conn);
    SPPlayableTrack(playable.id, trackId).save(conn);
}

} // namespace

int main() {
    const auto path = (std::filesystem::temp_directory_path() / "bungalow-music-entity-test.db").string();
    std::filesystem::remove(path);
    {
        Database db(path);

        db.upsertArtist("isni-abba", "abba");
        db.upsertArtist("isni-beatles", "Beatles");
        db.upsertArtist("local:cher", "Cher");
        db.upsertArtist("isni-nobody", "aardvark");

        db.upsertRecording("isrc-1", "Waterloo", "");
        db.upsertRecording("isrc-2", "help!", "Mono");
        db.upsertRecording("isrc-3", "Believe", "");
        db.upsertRecording("isrc-4", "Duet", "Live");
        db.upsertRecording("isrc-5", "Uncredited", "");

        db.upsertRelease("upc-a", "Zebra Hits", "1999-01-01");
        db.upsertRelease("upc-b", "alpha Songs", "1974-03-04");
        db.upsertRelease("upc-c", "Middle", "");

        db.setRecordingArtists("isrc-1", {{"isni-abba", "", true, 0}});
        db.setRecordingArtists("isrc-2", {{"isni-beatles", "", true, 0}});
        db.setRecordingArtists("isrc-3", {{"local:cher", "", true, 0}});
        // Credit order is by number, not by name.
        db.setRecordingArtists("isrc-4", {{"local:cher", "", true, 0}, {"isni-abba", "", false, 1}});
        db.setReleaseArtists("upc-a", {{"isni-beatles", "", true, 0}, {"isni-abba", "", false, 1}});
        db.setReleaseArtists("upc-b", {{"isni-abba", "", true, 0}});

        const int64_t waterlooOnAlpha = db.upsertTrack("isrc-1", "upc-b");
        const int64_t waterlooOnZebra = db.upsertTrack("isrc-1", "upc-a");
        const int64_t helpOnZebra     = db.upsertTrack("isrc-2", "upc-a");
        const int64_t believeLoose    = db.upsertTrack("isrc-3", "");
        const int64_t duetOnZebra     = db.upsertTrack("isrc-4", "upc-a");
        const int64_t duetOnMiddle    = db.upsertTrack("isrc-4", "upc-c");
        const int64_t duetLoose       = db.upsertTrack("isrc-4", "");
        db.upsertTrack("isrc-5", "upc-c");

        // Numbered tracks come first in number order; unnumbered ones follow as inserted.
        db.setTrackNumber(helpOnZebra, 1);
        db.setTrackNumber(duetOnZebra, 2);
        db.setTrackNumber(waterlooOnAlpha, 3);

        addPlayable(db, waterlooOnZebra, "file:///waterloo.mp3");
        addPlayable(db, duetOnMiddle, "file:///duet-first.mp3");
        addPlayable(db, duetOnMiddle, "file:///duet-second.mp3");
        addPlayable(db, believeLoose, "file:///believe.mp3");

        db.ensureSave(waterlooOnAlpha);
        db.ensureSave(duetLoose);
        db.ensureSave(helpOnZebra);
        db.removeSave(helpOnZebra);

        const MusicEntityRepository library(db);

        // ── Track lists ──────────────────────────────────────────────────────
        expect(describe(library.tracks()), describe(db.getAllTracks()), "all tracks");
        for (const char* upc : {"upc-a", "upc-b", "upc-c", "upc-missing"})
            expect(describe(library.tracksForRelease(upc)), describe(db.getTracksForRelease(upc)),
                   std::string("tracks for release ") + upc);
        for (const char* isni : {"isni-abba", "isni-beatles", "local:cher", "isni-nobody"})
            expect(describe(library.tracksForArtist(isni)), describe(db.getTracksForArtist(isni)),
                   std::string("tracks for artist ") + isni);
        for (const char* isrc : {"isrc-1", "isrc-3", "isrc-4", "isrc-5", "isrc-missing"})
            expect(describe(library.tracksForRecording(isrc)), describe(db.getTracksForRecording(isrc)),
                   std::string("tracks for recording ") + isrc);

        // ── Release lists ────────────────────────────────────────────────────
        expect(describe(library.releases()), describe(db.getReleaseSummaries()), "releases");
        for (const char* isni : {"isni-abba", "isni-beatles", "local:cher", "isni-nobody"})
            expect(describe(library.releasesForArtist(isni)), describe(db.getReleasesForArtist(isni)),
                   std::string("releases for artist ") + isni);

        // ── Artists ──────────────────────────────────────────────────────────
        std::string expected, actual;
        for (const auto& a : db.getArtistSummaries())
            expected += a.artist.isni + "|" + a.artist.name + "|" +
                        std::to_string(a.recordingCount) + "|" + std::to_string(a.releaseCount) + "\n";
        for (const auto& a : library.artists())
            actual += a->id() + "|" + a->text(MusicColumn::Name) + "|" +
                      a->text(MusicColumn::RecordingCount) + "|" + a->text(MusicColumn::ReleaseCount) + "\n";
        expect(actual, expected, "artists");

        // ── Recordings ───────────────────────────────────────────────────────
        expected.clear();
        actual.clear();
        for (const auto& r : db.getRecordingSummaries())
            expected += r.recording.isrc + "|" + r.recording.name + "|" + r.recording.version + "|" +
                        r.artists + "\n";
        for (const auto& r : library.recordings())
            actual += r->id() + "|" + r->text(MusicColumn::Name) + "|" + r->text(MusicColumn::Version) +
                      "|" + r->text(MusicColumn::Artists) + "\n";
        expect(actual, expected, "recordings");

        // A recording plays through the first of its tracks that has a playable.
        const auto duet = library.recording("isrc-4");
        expect(duet ? duet->playUri() : "", "file:///duet-first.mp3", "recording plays through its track");
        expect(duet ? std::to_string(duet->trackId()) : "", std::to_string(duetOnMiddle),
               "recording's playing track");
        const auto help = library.recording("isrc-2");
        expect(help && !help->isPlayable() ? "greyed" : "enabled", "greyed", "recording without a playable");
        for (const auto& r : library.recordings())
            if (r->id() == "isrc-4")
                expect(r->playUri(), "file:///duet-first.mp3", "listed recording plays through its track");

        // ── What each kind of entity answers ─────────────────────────────────
        const auto cher = library.artist("local:cher");
        expect(cher ? cher->text(MusicColumn::Isni) : "?", "", "generated ids are not shown");
        expect(cher ? cher->uri() : "", "musik:isni:local:cher", "artist uri");
        expect(cher && cher->isPlayable() ? "enabled" : "greyed", "enabled", "artist rows open a page");
        expect(library.artist("isni-missing") ? "found" : "none", "none", "missing artist");

        const auto loose = library.tracksForRecording("isrc-3");
        expect(loose.size() == 1 ? loose[0]->text(MusicColumn::Appearance) : "", "(no release)",
               "track without a release");
        expect(loose.size() == 1 && loose[0]->track().recording == &loose[0]->recording() ? "wired" : "loose",
               "wired", "track model points at its recording");
        expect(loose.size() == 1 ? loose[0]->uri() : "", "musik:isrc:isrc-3", "track uri");
    }
    std::filesystem::remove(path);

    if (failures == 0) std::cout << "All music entity tests passed\n";
    return failures == 0 ? 0 : 1;
}
