#include "app/library/MusicEditActions.h"

#include "BungalowApp.h"
#include "PlatformUi.h"

#include <algorithm>
#include <cctype>

namespace space::views::edit {

namespace {

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string trim(const std::string& s) {
    const auto first = s.find_first_not_of(" \t");
    if (first == std::string::npos) return "";
    return s.substr(first, s.find_last_not_of(" \t") - first + 1);
}

void showError(const std::string& message) {
    if (auto* app = BungalowApp::current())
        app->showError(message);
}

// The identifier for a new entity: the typed one, or a generated local id.
// Empty when the typed identifier is already taken.
std::string newKey(const std::string& typed, bool taken, const std::string& what) {
    if (typed.empty()) return Database::generateLocalId();
    if (taken) {
        showError("A " + what + " with the identifier " + typed + " already exists.");
        return "";
    }
    return typed;
}

constexpr const char* kIdentifierHint = "leave empty if unknown";
constexpr const char* kArtistsHint    = "comma-separated, primary first";

} // namespace

std::string joinArtistNames(const std::vector<Database::ArtistRelationship>& credits) {
    std::string out;
    for (const auto& c : credits) {
        if (!out.empty()) out += ", ";
        out += c.artistName;
    }
    return out;
}

std::vector<Database::ArtistRelationship> resolveArtistNames(Database& db, const std::string& csv) {
    std::vector<Database::ArtistRelationship> out;
    auto all = db.getAllArtists();

    size_t pos = 0;
    while (pos <= csv.size()) {
        const size_t comma = csv.find(',', pos);
        const std::string name = trim(csv.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos));
        pos = comma == std::string::npos ? csv.size() + 1 : comma + 1;
        if (name.empty()) continue;

        const bool collected = std::any_of(out.begin(), out.end(),
            [&name](const Database::ArtistRelationship& c) { return lower(c.artistName) == lower(name); });
        if (collected) continue;

        auto existing = std::find_if(all.begin(), all.end(),
            [&name](const Database::ArtistRecord& a) { return lower(a.name) == lower(name); });
        Database::ArtistRelationship credit;
        if (existing != all.end()) {
            credit.artistIsni = existing->isni;
            credit.artistName = existing->name;
        } else {
            credit.artistIsni = Database::generateLocalId();
            credit.artistName = name;
            db.upsertArtist(credit.artistIsni, credit.artistName);
            all.push_back({ credit.artistIsni, credit.artistName });
        }
        credit.number    = static_cast<int>(out.size());
        credit.isPrimary = out.empty();
        out.push_back(credit);
    }
    return out;
}

std::string artist(Database& db, const std::string& isni) {
    const bool adding = isni.empty();
    Database::ArtistRecord a = adding ? Database::ArtistRecord{} : db.getArtist(isni);
    if (!adding && a.isni.empty()) return "";

    std::vector<platform::FormField> fields = {
        { "ISNI", a.isni, adding ? kIdentifierHint : "", false, !adding },
        { "Name", a.name, "",                            true,  false   },
    };
    if (!platform::showFormDialog(adding ? "New Artist" : "Edit Artist", fields)) return "";

    std::string key = isni;
    if (adding) {
        key = newKey(fields[0].value, !db.getArtist(fields[0].value).isni.empty(), "artist");
        if (key.empty()) return "";
    }
    db.upsertArtist(key, fields[1].value);
    return key;
}

std::string recording(Database& db, const std::string& isrc) {
    const bool adding = isrc.empty();
    Database::RecordingRecord r = adding ? Database::RecordingRecord{} : db.getRecording(isrc);
    if (!adding && r.isrc.empty()) return "";

    std::vector<platform::FormField> fields = {
        { "ISRC",    r.isrc,    adding ? kIdentifierHint : "", false, !adding },
        { "Name",    r.name,    "",                            true,  false   },
        { "Version", r.version, "e.g. Radio Edit, Live",       false, false   },
        { "Artists", adding ? "" : joinArtistNames(db.getRecordingArtists(isrc)), kArtistsHint, false, false },
    };
    if (!platform::showFormDialog(adding ? "New Recording" : "Edit Recording", fields)) return "";

    std::string key = isrc;
    if (adding) {
        key = newKey(fields[0].value, !db.getRecording(fields[0].value).isrc.empty(), "recording");
        if (key.empty()) return "";
    }
    db.upsertRecording(key, fields[1].value, fields[2].value);
    db.setRecordingArtists(key, resolveArtistNames(db, fields[3].value));
    return key;
}

std::string release(Database& db, const std::string& upc) {
    const bool adding = upc.empty();
    Database::ReleaseRecord r = adding ? Database::ReleaseRecord{} : db.getRelease(upc);
    if (!adding && r.upc.empty()) return "";

    std::vector<platform::FormField> fields = {
        { "UPC",          r.upc,         adding ? kIdentifierHint : "",  false, !adding },
        { "Name",         r.name,        "",                             true,  false   },
        { "Release date", r.releaseDate, "YYYY-MM-DD, YYYY-MM or YYYY",  false, false   },
        { "Artists", adding ? "" : joinArtistNames(db.getReleaseArtists(upc)), kArtistsHint, false, false },
    };
    if (!platform::showFormDialog(adding ? "New Release" : "Edit Release", fields)) return "";

    std::string key = upc;
    if (adding) {
        key = newKey(fields[0].value, !db.getRelease(fields[0].value).upc.empty(), "release");
        if (key.empty()) return "";
    }
    db.upsertRelease(key, fields[1].value, fields[2].value);
    db.setReleaseArtists(key, resolveArtistNames(db, fields[3].value));
    return key;
}

bool addTrackToRelease(Database& db, const std::string& upc) {
    auto recordings = db.getRecordingSummaries();

    std::vector<std::string> choices = { "+ New recording..." };
    for (const auto& r : recordings) {
        std::string label = r.recording.name;
        if (!r.recording.version.empty()) label += " (" + r.recording.version + ")";
        if (!r.artists.empty())           label += "  -  " + r.artists;
        choices.push_back(label);
    }

    const int sel = platform::showChoiceDialog("Add Track", "Recording to add to this release:", choices);
    if (sel < 0) return false;

    std::string isrc;
    if (sel == 0) {
        isrc = recording(db);
        if (isrc.empty()) return false;
    } else {
        isrc = recordings[static_cast<size_t>(sel - 1)].recording.isrc;
    }

    // Number it after the release's current last track.
    int next = 1;
    for (const auto& t : db.getTracksForRelease(upc))
        if (t.number >= next) next = t.number + 1;

    const int64_t trackId = db.upsertTrack(isrc, upc);
    if (trackId < 0) return false;
    db.setTrackNumber(trackId, next);
    return true;
}

bool trackNumber(Database& db, int trackId, int current) {
    std::vector<platform::FormField> fields = {
        { "Track number", std::to_string(current), "0 = unnumbered", true, false },
    };
    if (!platform::showFormDialog("Track Number", fields)) return false;

    int n = 0;
    try {
        n = std::stoi(fields[0].value);
    } catch (...) {
        showError("\"" + fields[0].value + "\" is not a track number.");
        return false;
    }
    if (n < 0 || n == current) return false;
    db.setTrackNumber(trackId, n);
    return true;
}

} // namespace space::views::edit
