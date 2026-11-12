#include "MusicEditActions.h"

#include "Database.h"
#include "MusicEntityDialog.h"

#include <wx/choicdlg.h>
#include <wx/msgdlg.h>
#include <wx/numdlg.h>

namespace space::views::edit {

// The identifier for a new entity: the typed one, or a generated local id.
// Empty when the typed identifier is already taken.
static std::string newKey(wxWindow* parent, const std::string& typed, bool taken,
                          const std::string& what) {
    if (typed.empty()) return Database::generateLocalId();
    if (taken) {
        wxMessageBox(wxString::FromUTF8("A " + what + " with that identifier already exists."),
                     "Already exists", wxOK | wxICON_WARNING, parent);
        return "";
    }
    return typed;
}

std::string artist(wxWindow* parent, Database& db, const std::string& isni) {
    const bool adding = isni.empty();
    Database::ArtistRecord a = adding ? Database::ArtistRecord{} : db.getArtist(isni);
    if (!adding && a.isni.empty()) return "";

    MusicEntityDialog dlg(parent, adding ? "New Artist" : "Edit Artist", {
        { "ISNI", a.isni, adding ? "Leave empty if unknown" : "", false, !adding },
        { "Name", a.name, "",                                      true,  false   },
    });
    if (dlg.ShowModal() != wxID_OK) return "";

    std::string key = isni;
    if (adding) {
        key = newKey(parent, dlg.value(0), !db.getArtist(dlg.value(0)).isni.empty(), "artist");
        if (key.empty()) return "";
    }
    db.upsertArtist(key, dlg.value(1));
    return key;
}

std::string recording(wxWindow* parent, Database& db, const std::string& isrc) {
    const bool adding = isrc.empty();
    Database::RecordingRecord r = adding ? Database::RecordingRecord{} : db.getRecording(isrc);
    if (!adding && r.isrc.empty()) return "";

    auto allArtists = db.getAllArtists();
    auto credited   = adding ? std::vector<Database::ArtistRelationship>{}
                             : db.getRecordingArtists(isrc);
    MusicEntityDialog dlg(parent, adding ? "New Recording" : "Edit Recording", {
        { "ISRC",    r.isrc,    adding ? "Leave empty if unknown" : "", false, !adding },
        { "Name",    r.name,    "",                                      true,  false   },
        { "Version", r.version, "e.g. Radio Edit, Live",                 false, false   },
    }, &allArtists, credited);
    if (dlg.ShowModal() != wxID_OK) return "";

    std::string key = isrc;
    if (adding) {
        key = newKey(parent, dlg.value(0), !db.getRecording(dlg.value(0)).isrc.empty(), "recording");
        if (key.empty()) return "";
    }
    db.upsertRecording(key, dlg.value(1), dlg.value(2));
    db.setRecordingArtists(key, dlg.collectArtists(db));
    return key;
}

std::string release(wxWindow* parent, Database& db, const std::string& upc) {
    const bool adding = upc.empty();
    Database::ReleaseRecord r = adding ? Database::ReleaseRecord{} : db.getRelease(upc);
    if (!adding && r.upc.empty()) return "";

    auto allArtists = db.getAllArtists();
    auto credited   = adding ? std::vector<Database::ArtistRelationship>{}
                             : db.getReleaseArtists(upc);
    MusicEntityDialog dlg(parent, adding ? "New Release" : "Edit Release", {
        { "UPC",          r.upc,         adding ? "Leave empty if unknown" : "", false, !adding },
        { "Name",         r.name,        "",                                      true,  false   },
        { "Release date", r.releaseDate, "YYYY-MM-DD, YYYY-MM or YYYY",           false, false   },
    }, &allArtists, credited);
    if (dlg.ShowModal() != wxID_OK) return "";

    std::string key = upc;
    if (adding) {
        key = newKey(parent, dlg.value(0), !db.getRelease(dlg.value(0)).upc.empty(), "release");
        if (key.empty()) return "";
    }
    db.upsertRelease(key, dlg.value(1), dlg.value(2));
    db.setReleaseArtists(key, dlg.collectArtists(db));
    return key;
}

bool addTrackToRelease(wxWindow* parent, Database& db, const std::string& upc) {
    auto recordings = db.getRecordingSummaries();

    wxArrayString choices;
    choices.Add(wxString::FromUTF8("＋ New recording…"));
    for (const auto& r : recordings) {
        std::string label = r.recording.name;
        if (!r.recording.version.empty()) label += " (" + r.recording.version + ")";
        if (!r.artists.empty())           label += "  —  " + r.artists;
        choices.Add(wxString::FromUTF8(label));
    }

    wxSingleChoiceDialog pick(parent, "Recording to add to this release:", "Add Track", choices);
    if (pick.ShowModal() != wxID_OK) return false;

    std::string isrc;
    int sel = pick.GetSelection();
    if (sel == 0) {
        isrc = recording(parent, db);
        if (isrc.empty()) return false;
    } else {
        isrc = recordings[static_cast<size_t>(sel - 1)].recording.isrc;
    }

    // Number it after the release's current last track.
    int next = 1;
    for (const auto& t : db.getTracksForRelease(upc))
        if (t.number >= next) next = t.number + 1;

    int64_t trackId = db.upsertTrack(isrc, upc);
    if (trackId < 0) return false;
    db.setTrackNumber(trackId, next);
    return true;
}

bool trackNumber(wxWindow* parent, Database& db, int trackId, int current) {
    long n = wxGetNumberFromUser("Position of this track on its release (0 = unnumbered).",
                                 "Track number:", "Track Number", current, 0, 999, parent);
    if (n < 0 || n == current) return false;
    db.setTrackNumber(trackId, static_cast<int>(n));
    return true;
}

} // namespace space::views::edit
