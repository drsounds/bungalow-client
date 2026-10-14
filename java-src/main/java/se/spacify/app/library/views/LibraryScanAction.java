package se.spacify.app.library.views;

import se.spacify.library.MusicScanner;
import se.spacify.library.ScanResult;

import javax.swing.*;
import java.awt.*;
import java.io.File;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Drives the "Scan…" toolbar flow: pick a folder, scan it on a background
 * thread behind a modal progress dialog, then show a summary and refresh.
 */
public final class LibraryScanAction {

    private LibraryScanAction() {}

    public static void run(Component parent, Runnable onComplete) {
        JFileChooser chooser = new JFileChooser();
        chooser.setDialogTitle("Select a folder to scan for MP3 files");
        chooser.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
        if (chooser.showOpenDialog(parent) != JFileChooser.APPROVE_OPTION) return;
        File folder = chooser.getSelectedFile();

        Window owner = SwingUtilities.getWindowAncestor(parent);
        JDialog dialog = new JDialog(owner, "Scanning…", Dialog.ModalityType.APPLICATION_MODAL);
        JProgressBar bar = new JProgressBar(0, 100);
        bar.setStringPainted(true);
        bar.setIndeterminate(true);
        JLabel label = new JLabel("Preparing…");
        label.setBorder(BorderFactory.createEmptyBorder(0, 0, 8, 0));
        JButton cancel = new JButton("Cancel");

        JPanel content = new JPanel(new BorderLayout(0, 8));
        content.setBorder(BorderFactory.createEmptyBorder(16, 16, 16, 16));
        content.add(label, BorderLayout.NORTH);
        content.add(bar, BorderLayout.CENTER);
        JPanel south = new JPanel(new FlowLayout(FlowLayout.RIGHT, 0, 0));
        south.add(cancel);
        content.add(south, BorderLayout.SOUTH);
        dialog.setContentPane(content);
        dialog.setSize(440, 150);
        dialog.setLocationRelativeTo(parent);

        AtomicBoolean cancelled = new AtomicBoolean(false);
        cancel.addActionListener(e -> { cancelled.set(true); label.setText("Cancelling…"); });

        SwingWorker<ScanResult, Object[]> worker = new SwingWorker<>() {
            @Override protected ScanResult doInBackground() {
                return new MusicScanner().scan(folder, new MusicScanner.ProgressCallback() {
                    @Override public void onProgress(int done, int total, String currentFile) {
                        publish(new Object[]{done, total, currentFile});
                    }
                    @Override public boolean isCancelled() { return cancelled.get(); }
                });
            }

            @Override protected void process(List<Object[]> chunks) {
                Object[] last = chunks.get(chunks.size() - 1);
                int done = (int) last[0], total = (int) last[1];
                String file = (String) last[2];
                if (total > 0) {
                    bar.setIndeterminate(false);
                    bar.setValue((int) (done * 100.0 / total));
                    label.setText("Scanning " + done + " / " + total
                        + (file.isEmpty() ? "" : ":  " + file));
                } else {
                    label.setText("No MP3 files found");
                }
            }

            @Override protected void done() {
                dialog.dispose();
                ScanResult result;
                try {
                    result = get();
                } catch (Exception e) {
                    JOptionPane.showMessageDialog(parent, "Scan failed: " + e.getMessage(),
                        "Scan error", JOptionPane.ERROR_MESSAGE);
                    return;
                }
                if (onComplete != null) onComplete.run();
                JTextArea area = new JTextArea(result.summary());
                area.setEditable(false);
                area.setOpaque(false);
                JOptionPane.showMessageDialog(parent, area, "Scan complete",
                    JOptionPane.INFORMATION_MESSAGE);
            }
        };

        worker.execute();
        dialog.setVisible(true); // blocks until dialog.dispose() in done()
    }
}
