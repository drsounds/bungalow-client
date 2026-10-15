-- ───────────────────────────────────────────────────────────────────────────
-- Spacify library DB migration: legacy "title" schema → Node "name" schema.
--
-- Brings a pre-refactor ~/.spacify/library.db up to the entity model introduced
-- with the Node/Content/Creator abstraction + playlists:
--   * title → name on recordings / releases / music_works
--   * new version column on every Node (recordings / releases / music_works / artists)
--   * local file paths detached from recordings into their own recording_files table
--   * row_index added to tracks (ContentCollectionRow position)
--   * new playlists / playlist_rows / recording_files tables
--
-- One-shot: re-running will fail on the RENAME/ADD COLUMN steps (a column is
-- renamed/added only once). BACK UP FIRST:
--     cp ~/.spacify/library.db ~/.spacify/library.db.bak
-- Then run:
--     sqlite3 ~/.spacify/library.db < docs/migrate-legacy-db.sql
-- Requires SQLite ≥ 3.35 (RENAME/DROP COLUMN).
-- ───────────────────────────────────────────────────────────────────────────

PRAGMA foreign_keys = OFF;
BEGIN TRANSACTION;

-- ── New tables (column names mirror the ORMLite entity mappings) ─────────────
CREATE TABLE IF NOT EXISTS recording_files (
    id           INTEGER PRIMARY KEY AUTOINCREMENT,
    recording_id INTEGER NOT NULL,
    file_path    VARCHAR NOT NULL
);

CREATE TABLE IF NOT EXISTS playlists (
    id      INTEGER PRIMARY KEY AUTOINCREMENT,
    name    VARCHAR NOT NULL,
    version INTEGER,
    uuid    VARCHAR NOT NULL UNIQUE
);

CREATE TABLE IF NOT EXISTS playlist_rows (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    row_index     INTEGER,
    playlist_id   INTEGER NOT NULL,
    playlist_name VARCHAR,
    content_uri   VARCHAR NOT NULL,
    title         VARCHAR,
    artist        VARCHAR,
    duration_ms   BIGINT
);

-- ── Detach local file paths from recordings into recording_files ─────────────
INSERT INTO recording_files (recording_id, file_path)
    SELECT id, filePath FROM recordings
    WHERE filePath IS NOT NULL AND filePath <> '';

-- ── title → name, plus the Node version column ───────────────────────────────
ALTER TABLE recordings  RENAME COLUMN title TO name;
ALTER TABLE recordings  ADD COLUMN version INTEGER NOT NULL DEFAULT 0;
ALTER TABLE releases    RENAME COLUMN title TO name;
ALTER TABLE releases    ADD COLUMN version INTEGER NOT NULL DEFAULT 0;
ALTER TABLE music_works RENAME COLUMN title TO name;
ALTER TABLE music_works ADD COLUMN version INTEGER NOT NULL DEFAULT 0;
ALTER TABLE artists     ADD COLUMN version INTEGER NOT NULL DEFAULT 0;

-- ── ContentCollectionRow position for tracks ─────────────────────────────────
ALTER TABLE tracks ADD COLUMN row_index INTEGER NOT NULL DEFAULT 0;

-- ── Drop the now-detached filePath column ────────────────────────────────────
-- Safe to keep if your SQLite predates DROP COLUMN (3.35): the new code never
-- reads it. To keep it, comment the next line out.
ALTER TABLE recordings DROP COLUMN filePath;

COMMIT;
PRAGMA foreign_keys = ON;

-- recording_artist_credits / release_artist_credits are unchanged: their
-- columns (id, recording_id/release_id, artist_id, primary, role) already match
-- the new CreatorCredit hierarchy.
