package se.spacify.app.youtube;

import org.cef.CefClient;
import org.cef.browser.CefBrowser;
import org.cef.browser.CefFrame;
import org.cef.browser.CefMessageRouter;
import org.cef.callback.CefCallback;
import org.cef.callback.CefQueryCallback;
import org.cef.callback.CefSchemeHandlerFactory;
import org.cef.handler.CefMessageRouterHandlerAdapter;
import org.cef.handler.CefResourceHandler;
import org.cef.handler.CefResourceHandlerAdapter;
import org.cef.misc.IntRef;
import org.cef.misc.StringRef;
import org.cef.network.CefRequest;
import org.cef.network.CefResponse;

import se.spacify.app.media.service.MediaServicePlayerComponent;
import se.spacify.ui.theme.ThemeManager;
import se.spacify.web.CefRuntime;

import javax.swing.BorderFactory;
import javax.swing.JLabel;
import javax.swing.SwingConstants;
import javax.swing.SwingWorker;
import java.awt.BorderLayout;
import java.awt.Dimension;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * The YouTube playback surface: an embedded CEF browser running a small page that
 * drives the <a href="https://developers.google.com/youtube/iframe_api_reference">
 * YouTube IFrame Player API</a>. {@link #loadAndPlay(String)} resolves a query to
 * video ids via {@link YouTubeSearch} (no API key) and plays the first with
 * {@code loadVideoById}, falling through to the next on an embedding/playback
 * error — the IFrame API's {@code listType:'search'} loader was removed by
 * YouTube. Player state and position are posted back from JavaScript through a
 * {@link CefMessageRouter} ({@code window.cefQuery}) and forwarded to the owning
 * {@link YouTubeMusicService}.
 *
 * <p>The host page is served from a registered {@code https://spacify.youtube}
 * scheme handler rather than {@code file://}, so the IFrame API sees a real,
 * secure origin — embedding from a {@code null} (file) origin makes YouTube
 * refuse playback ("Video player configuration error").
 *
 * <p>CEF is built lazily off the EDT on first use (the first run downloads
 * Chromium); calls made before the player is ready are queued.
 */
public class YouTubePlayerComponent extends MediaServicePlayerComponent {

    private static final String SCHEME   = "https";
    private static final String DOMAIN   = "spacify.youtube";
    private static final String PAGE_URL = SCHEME + "://" + DOMAIN + "/player.html";
    
    private static final byte[] PAGE_BYTES = page().getBytes(StandardCharsets.UTF_8);

    /** The page handler factory is process-wide; register it at most once. */
    private static final AtomicBoolean schemeRegistered = new AtomicBoolean(false);

    private final YouTubeMusicService Service;
    private final JLabel status;

    private CefClient  client;
    private CefBrowser browser;
    private boolean    initStarted;
    private boolean    ready;
    // Resolved video ids for the current query; we try the next one if a video
    // turns out to be non-embeddable / region-blocked.
    private java.util.List<String> resultIds = java.util.List.of();
    private int     resultIndex;
    private String  pendingPlayId;   // id to play once the IFrame player is ready

    public YouTubePlayerComponent(YouTubeMusicService Service) {
        this.Service = Service;
        getComponent().setLayout(new BorderLayout());
        getComponent().setPreferredSize(new Dimension(0, 200));
        status = new JLabel("YouTube", SwingConstants.CENTER);
        status.setForeground(ThemeManager.getForeground());
        status.setBorder(BorderFactory.createEmptyBorder(24, 12, 24, 12));
        getComponent().add(status, BorderLayout.CENTER);
    }

    // ── Playback API used by the Service ────────────────────────────────────────

    /**
     * Resolve {@code query} to YouTube video ids (off the EDT) and play the first
     * playable one. {@code query} may already be a bare 11-char video id. The
     * IFrame API's {@code listType:"search"} loader was removed by YouTube, so we
     * search ourselves and {@code loadVideoById}.
     */
    public void loadAndPlay(String query) {
        ensureBrowser();
        if (YouTubeSearch.isVideoId(query)) {
            setResults(java.util.List.of(query));
            return;
        }
        String apiKey = Service.apiKey();
        new SwingWorker<java.util.List<String>, Void>() {
            @Override protected java.util.List<String> doInBackground() {
                return YouTubeSearch.search(query, apiKey).stream().map(YouTubeSearch.Result::videoId).toList();
            }
            @Override protected void done() {
                try { setResults(get()); } catch (Exception e) { setResults(java.util.List.of()); }
            }
        }.execute();
    }

    /** Adopt a fresh list of candidate video ids and start playing the first. */
    private void setResults(java.util.List<String> ids) {
        resultIds = ids;
        resultIndex = 0;
        if (!ids.isEmpty()) playCurrentResult();
    }

    /** Play the current candidate, or queue it until the player reports ready. */
    private void playCurrentResult() {
        if (resultIndex < 0 || resultIndex >= resultIds.size()) return;
        String id = resultIds.get(resultIndex);
        if (ready) runJs("playVideoId(" + jsString(id) + ")");
        else       pendingPlayId = id;
    }

    public void play()  { runJs("doPlay()"); }
    public void pause() { runJs("doPause()"); }
    public void seekTo(long positionMs) { runJs("doSeek(" + (positionMs / 1000.0) + ")"); }

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    @Override
    public void onActivated() {
        ensureBrowser();
    }

    // ── CEF setup ───────────────────────────────────────────────────────────────

    private void ensureBrowser() {
        if (initStarted) return;
        initStarted = true;
        status.setText("Starting YouTube player…");

        new SwingWorker<CefClient, Void>() {
            @Override protected CefClient doInBackground() throws Exception {
                CefClient c = CefRuntime.newClient();   // heavy on first call
                // Serve the IFrame-API host page from a real https origin.
                if (schemeRegistered.compareAndSet(false, true)) {
                    CefRuntime.registerSchemeHandlerFactory(SCHEME, DOMAIN, new PageFactory());
                }
                return c;
            }
            @Override protected void done() {
                try {
                    client = get();
                    attachBrowser();
                } catch (Exception e) {
                    initStarted = false;
                    schemeRegistered.set(false);
                    status.setText("<html>Could not start the YouTube player:<br>"
                        + e.getMessage() + "</html>");
                }
            }
        }.execute();
    }

    private void attachBrowser() {
        // Receive state/position/title messages posted from the page via cefQuery.
        CefMessageRouter router = CefMessageRouter.create();
        router.addHandler(new CefMessageRouterHandlerAdapter() {
            @Override
            public boolean onQuery(CefBrowser b, CefFrame f, long id, String request,
                                   boolean persistent, CefQueryCallback callback) {
                handleMessage(request);
                callback.success("");
                return true;
            }
        }, true);
        client.addMessageRouter(router);

        browser = client.createBrowser(PAGE_URL, false, false);
        getComponent().remove(status);
        getComponent().add(browser.getUIComponent(), BorderLayout.CENTER);
        revalidate();
        repaint();
    }

    /** Parse a {@code key:value} message posted from the page and forward it. */
    private void handleMessage(String request) {
        if (request == null) return;
        int sep = request.indexOf(':');
        String key = sep >= 0 ? request.substring(0, sep) : request;
        String val = sep >= 0 ? request.substring(sep + 1) : "";
        switch (key) {
            case "ready" -> {
                ready = true;
                if (pendingPlayId != null) {
                    runJs("playVideoId(" + jsString(pendingPlayId) + ")");
                    pendingPlayId = null;
                }
            }
            case "error" -> {
                // 2 = bad id, 5 = HTML5 error, 100 = removed/private,
                // 101/150 = embedding disabled → fall through to the next result.
                resultIndex++;
                if (resultIndex < resultIds.size()) playCurrentResult();
            }
            case "state" -> {
                try { Service.onPlayerState(Integer.parseInt(val.trim())); }
                catch (NumberFormatException ignored) {}
            }
            case "time" -> {
                int c = val.indexOf(':');
                if (c >= 0) {
                    double cur = parseD(val.substring(0, c));
                    double dur = parseD(val.substring(c + 1));
                    Service.onPlayerTime(cur, dur);
                }
            }
            case "title" -> Service.onPlayerTitle(val);
            default -> { /* ignore */ }
        }
    }

    private void runJs(String code) {
        if (browser != null) browser.executeJavaScript(code, browser.getURL(), 0);
    }

    // ── Helpers ───────────────────────────────────────────────────────────────

    private static double parseD(String s) {
        try { return Double.parseDouble(s.trim()); } catch (NumberFormatException e) { return 0; }
    }

    /** Minimal JS string literal: wrap in quotes and escape backslash/quote/newline. */
    private static String jsString(String s) {
        String esc = s == null ? "" : s
            .replace("\\", "\\\\")
            .replace("\"", "\\\"")
            .replace("\n", " ")
            .replace("\r", " ");
        return "\"" + esc + "\"";
    }

    // ── Custom-scheme page serving ────────────────────────────────────────────

    /** Serves {@link #PAGE_BYTES} for any request to the registered scheme/domain. */
    private static final class PageFactory implements CefSchemeHandlerFactory {
        @Override
        public CefResourceHandler create(CefBrowser browser, CefFrame frame,
                                         String schemeName, CefRequest request) {
            return new PageHandler();
        }
    }

    /** In-memory resource handler streaming the host page as text/html. */
    private static final class PageHandler extends CefResourceHandlerAdapter {
        private int offset = 0;

        @Override
        public boolean processRequest(CefRequest request, CefCallback callback) {
            callback.Continue();
            return true;
        }

        @Override
        public void getResponseHeaders(CefResponse response, IntRef responseLength, StringRef redirectUrl) {
            response.setMimeType("text/html");
            response.setStatus(200);
            responseLength.set(PAGE_BYTES.length);
        }

        @Override
        public boolean readResponse(byte[] dataOut, int bytesToRead, IntRef bytesRead, CefCallback callback) {
            if (offset >= PAGE_BYTES.length) {
                bytesRead.set(0);
                return false;
            }
            int n = Math.min(bytesToRead, PAGE_BYTES.length - offset);
            System.arraycopy(PAGE_BYTES, offset, dataOut, 0, n);
            offset += n;
            bytesRead.set(n);
            return true;
        }
    }

    /** The IFrame-API host page. {@code origin} is set so the API accepts our scheme. */
    private static String page() {
        return """
            <!DOCTYPE html>
            <html>
            <head><meta charset="utf-8">
            <style>html,body{margin:0;height:100%;background:#000;overflow:hidden}#player{width:100%;height:100%}</style>
            </head>
            <body>
              <div id="player"></div>
              <script src="https://www.youtube.com/iframe_api"></script>
              <script>
                var player;
                function post(msg){ try{ if(window.cefQuery) window.cefQuery({request:msg,onSuccess:function(){},onFailure:function(){}});}catch(e){} }
                function onYouTubeIframeAPIReady(){
                  player = new YT.Player('player', {
                    height:'100%', width:'100%',
                    playerVars:{autoplay:1, controls:1, modestbranding:1, rel:0, playsinline:1, origin:window.location.origin},
                    events:{
                      'onReady':function(){ post('ready:'); },
                      'onError':function(e){ post('error:'+e.data); },
                      'onStateChange':function(e){
                        post('state:'+e.data);
                        if(e.data==1 && player.getVideoData){ post('title:'+(player.getVideoData().title||'')); }
                      }
                    }
                  });
                }
                // listType:'search' was removed from the IFrame API; play concrete ids.
                function playVideoId(id){ if(player&&player.loadVideoById) player.loadVideoById(id); }
                function doPlay(){ if(player&&player.playVideo) player.playVideo(); }
                function doPause(){ if(player&&player.pauseVideo) player.pauseVideo(); }
                function doSeek(s){ if(player&&player.seekTo) player.seekTo(s,true); }
                setInterval(function(){ if(player&&player.getCurrentTime){ post('time:'+player.getCurrentTime()+':'+(player.getDuration?player.getDuration():0)); } }, 1000);
              </script>
            </body>
            </html>
            """;
    }
}
