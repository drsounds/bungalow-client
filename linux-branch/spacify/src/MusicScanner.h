#pragma once

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/thread.h>
#include <wx/gauge.h>

#include <sqlite3.h>
#include <atomic>
#include <string>

// ── Custom events (posted from worker thread → dialog) ────────────────────────
wxDECLARE_EVENT(EVT_SCAN_PROGRESS, wxCommandEvent);
wxDECLARE_EVENT(EVT_SCAN_DONE,     wxCommandEvent);

// ── Raw metadata extracted from an audio file ─────────────────────────────────
struct AudioMetadata {
    std::string title;    // cleaned title (version suffix removed)
    std::string version;  // extracted version string, e.g. "Radio Edit"
    std::string artists;  // comma-separated artist string from tag
    std::string album;
    int         duration = 0; // milliseconds (from TLEN or 0)
};

// ── Forward declarations ───────────────────────────────────────────────────────
class MusicScanDialog;

// ── Worker thread ─────────────────────────────────────────────────────────────
class MusicScannerThread : public wxThread {
public:
    MusicScannerThread(MusicScanDialog*    dlg,
                       const std::string& rootPath,
                       const std::string& dbPath);

protected:
    wxThread::ExitCode Entry() override;

private:
    MusicScanDialog* m_dialog;
    std::string      m_rootPath;
    std::string      m_dbPath;

    // ── File classification ──────────────────────────────────────────────────
    static bool isAudioFile(const std::string& ext);
    static bool isTaggableAudio(const std::string& ext); // supports ID3 / iTunes atoms

    // ── Metadata extraction ──────────────────────────────────────────────────
    AudioMetadata readTags(const std::string& path);
    AudioMetadata readID3v2(const std::string& path);
    AudioMetadata readID3v1(const std::string& path);

    // ── Version extraction (mutates title, returns version string) ───────────
    static std::string extractVersion(std::string& title);

    // ── Database helpers (thread-local connection) ───────────────────────────
    static sqlite3_int64 upsertFile(sqlite3* db, const std::string& path);
    static sqlite3_int64 upsertAudioFile(sqlite3* db, sqlite3_int64 fileId,
                                          const AudioMetadata& meta);
    static sqlite3_int64 upsertPlayable(sqlite3* db, const std::string& path);
    static bool matchAndLink(sqlite3* db, const AudioMetadata& meta,
                              sqlite3_int64 audioFileId, sqlite3_int64 playableId);

    // ── MusicBrainz helpers ──────────────────────────────────────────────────
    bool musicBrainzLookup(sqlite3* db, const AudioMetadata& meta,
                            sqlite3_int64 audioFileId, sqlite3_int64 playableId);
    static bool createFromMBJson(sqlite3* db, const std::string& jsonStr,
                                  const AudioMetadata& localMeta,
                                  sqlite3_int64 audioFileId, sqlite3_int64 playableId);
    static std::string urlEncode(const std::string& s);
    static std::string httpGet(const std::string& url);

    // ── Progress ─────────────────────────────────────────────────────────────
    void postProgress(int current, int total, const std::string& file, int imported);
    void postDone(int total, int imported);
};

// ── Scan progress dialog ──────────────────────────────────────────────────────
class MusicScanDialog : public wxDialog {
public:
    MusicScanDialog(wxWindow*          parent,
                    const std::string& folderPath,
                    const std::string& dbPath);
    ~MusicScanDialog() override;

private:
    wxStaticText*       m_folderLabel;
    wxStaticText*       m_fileLabel;
    wxGauge*            m_gauge;
    wxStaticText*       m_statsLabel;
    wxButton*           m_actionBtn; // "Cancel" while scanning, "Close" when done
    bool                m_scanDone = false;
    MusicScannerThread* m_thread   = nullptr;

    void OnProgress(wxCommandEvent& e);
    void OnDone(wxCommandEvent& e);
    void OnActionBtn(wxCommandEvent& e);
    void OnClose(wxCloseEvent& e);
};
