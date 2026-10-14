package se.spacify.app.downloads.model;

import com.j256.ormlite.field.DatabaseField;
import com.j256.ormlite.table.DatabaseTable;

/**
 * A persisted download record shown in the Downloads view. Captures a file
 * pulled from a store/web view (preserving the browser's cookies/session) and
 * its progress; on completion a capture scope imports it (audio → the library).
 * History survives restarts, so a paid download can be retried.
 */
@DatabaseTable(tableName = "downloads")
public class Download {

    public enum Status { QUEUED, DOWNLOADING, COMPLETE, FAILED, CANCELLED }

    @DatabaseField(generatedId = true)
    private int id;

    @DatabaseField(canBeNull = false)
    private String url;

    @DatabaseField(columnName = "file_name")
    private String fileName;

    @DatabaseField(columnName = "file_path")
    private String filePath;

    @DatabaseField(columnName = "mime_type")
    private String mimeType;

    /** Persisted as the enum name. */
    @DatabaseField(canBeNull = false)
    private String status = Status.QUEUED.name();

    /** Id of the capture scope that claimed this download (e.g. "music"). */
    @DatabaseField(columnName = "scope_id")
    private String scopeId;

    @DatabaseField(columnName = "received_bytes")
    private long receivedBytes;

    @DatabaseField(columnName = "total_bytes")
    private long totalBytes;

    // Denormalised display metadata, filled in once the file is tagged on import.
    @DatabaseField private String title;
    @DatabaseField private String artist;
    @DatabaseField private String album;

    @DatabaseField(columnName = "created_at")
    private long createdAt;

    @DatabaseField(columnName = "completed_at")
    private long completedAt;

    public Download() {}

    public Download(String url, String fileName, String filePath, String mimeType, String scopeId) {
        this.url = url;
        this.fileName = fileName;
        this.filePath = filePath;
        this.mimeType = mimeType;
        this.scopeId = scopeId;
        this.createdAt = System.currentTimeMillis();
    }

    public int     getId()            { return id; }
    public String  getUrl()           { return url; }
    public String  getFileName()      { return fileName; }
    public void    setFileName(String v) { this.fileName = v; }
    public String  getFilePath()      { return filePath; }
    public void    setFilePath(String v) { this.filePath = v; }
    public String  getMimeType()      { return mimeType; }
    public String  getScopeId()       { return scopeId; }

    public Status  getStatus()        { return Status.valueOf(status); }
    public void    setStatus(Status s){ this.status = s.name(); }

    public long    getReceivedBytes() { return receivedBytes; }
    public void    setReceivedBytes(long v) { this.receivedBytes = v; }
    public long    getTotalBytes()    { return totalBytes; }
    public void    setTotalBytes(long v) { this.totalBytes = v; }

    public String  getName()         { return title; }
    public void    setName(String v) { this.title = v; }
    public String  getArtist()        { return artist; }
    public void    setArtist(String v){ this.artist = v; }
    public String  getAlbum()         { return album; }
    public void    setAlbum(String v) { this.album = v; }

    public long    getCreatedAt()     { return createdAt; }
    public long    getCompletedAt()   { return completedAt; }
    public void    setCompletedAt(long v) { this.completedAt = v; }

    /** Percent complete (0–100), or -1 when the total size is unknown. */
    public int percent() {
        if (totalBytes <= 0) return -1;
        return (int) Math.min(100, receivedBytes * 100 / totalBytes);
    }
}
