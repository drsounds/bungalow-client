package se.spacify.app.music.model;

import com.j256.ormlite.field.DatabaseField;
import com.j256.ormlite.table.DatabaseTable;

/**
 * A local audio file backing a {@link Recording}, detached from the recording
 * itself so that local playback is treated as just another "music service" —
 * equal footing with YouTube, a streaming account, etc. A recording may have
 * zero or more files (e.g. different encodings).
 */
@DatabaseTable(tableName = "recording_files")
public class RecordingFile {

    @DatabaseField(generatedId = true)
    private int id;

    @DatabaseField(foreign = true, foreignAutoRefresh = true, canBeNull = false, columnName = "recording_id")
    private Recording recording;

    @DatabaseField(canBeNull = false, columnName = "file_path")
    private String filePath;

    public RecordingFile() {}

    public RecordingFile(Recording recording, String filePath) {
        this.recording = recording;
        this.filePath  = filePath;
    }

    public int       getId()                   { return id; }
    public Recording getRecording()            { return recording; }
    public void      setRecording(Recording v) { this.recording = v; }
    public String    getFilePath()             { return filePath; }
    public void      setFilePath(String v)     { this.filePath = v; }
}
