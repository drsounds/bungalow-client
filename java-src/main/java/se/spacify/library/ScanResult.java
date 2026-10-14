package se.spacify.library;

import java.util.ArrayList;
import java.util.List;

/** Tallies what a {@link MusicScanner} run produced. */
public class ScanResult {
    public int filesScanned;
    public int localFilesAdded;
    public int recordingsAdded;
    public int releasesAdded;
    public int artistsAdded;
    public int tracksAdded;
    public final List<String> errors = new ArrayList<>();

    public String summary() {
        StringBuilder sb = new StringBuilder();
        sb.append("Scanned ").append(filesScanned).append(" MP3 file(s).\n\n")
          .append("Added:\n")
          .append("  ").append(localFilesAdded).append(" local file(s)\n")
          .append("  ").append(recordingsAdded).append(" recording(s)\n")
          .append("  ").append(tracksAdded).append(" track(s)\n")
          .append("  ").append(releasesAdded).append(" release(s)\n")
          .append("  ").append(artistsAdded).append(" artist(s)");
        if (!errors.isEmpty()) {
            sb.append("\n\n").append(errors.size()).append(" file(s) could not be read:");
            int shown = Math.min(errors.size(), 10);
            for (int i = 0; i < shown; i++) sb.append("\n  ").append(errors.get(i));
            if (errors.size() > shown) sb.append("\n  …and ").append(errors.size() - shown).append(" more");
        }
        return sb.toString();
    }
}
