#include "LocalMediaService.h"
#include <gst/gst.h>
#include <string>

// ── GStreamer state ───────────────────────────────────────────────────────────

static GstElement* s_pipeline = nullptr;

static void ensurePipeline() {
    if (s_pipeline) return;
    gst_init(nullptr, nullptr);
    s_pipeline = gst_element_factory_make("playbin", "spacify-player");
}

// ── Helpers ───────────────────────────────────────────────────────────────────

/// Convert a file:// URI or plain path to a GStreamer-compatible URI.
/// GStreamer requires the full "file:///..." form.
static std::string toGstUri(const std::string& uri) {
    if (uri.rfind("file://", 0) == 0)
        return uri;
    return "file://" + uri;
}

static gint64 queryPosition() {
    gint64 pos = 0;
    gst_element_query_position(s_pipeline, GST_FORMAT_TIME, &pos);
    return pos;
}

static gint64 queryDuration() {
    gint64 dur = 0;
    gst_element_query_duration(s_pipeline, GST_FORMAT_TIME, &dur);
    return dur;
}

// ── SPService ─────────────────────────────────────────────────────────────────

bool LocalMediaService::acceptsUri(const std::string& uri) const {
    return uri.rfind("file://", 0) == 0;
}

// ── SPMediaService ────────────────────────────────────────────────────────────

void LocalMediaService::load(const std::string& uri) {
    m_loadedUri = uri;
    m_playing   = false;
    ensurePipeline();
    gst_element_set_state(s_pipeline, GST_STATE_NULL);
    g_object_set(s_pipeline, "uri", toGstUri(uri).c_str(), nullptr);
    gst_element_set_state(s_pipeline, GST_STATE_PAUSED);
}

void LocalMediaService::play() {
    if (m_loadedUri.empty() || !s_pipeline) return;
    m_playing = true;
    gst_element_set_state(s_pipeline, GST_STATE_PLAYING);
}

void LocalMediaService::pause() {
    m_playing = false;
    if (s_pipeline)
        gst_element_set_state(s_pipeline, GST_STATE_PAUSED);
}

void LocalMediaService::stop() {
    m_playing = false;
    if (s_pipeline)
        gst_element_set_state(s_pipeline, GST_STATE_NULL);
}

void LocalMediaService::seek(int posMs) {
    if (!s_pipeline) return;
    gst_element_seek_simple(
        s_pipeline,
        GST_FORMAT_TIME,
        static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT),
        static_cast<gint64>(posMs) * GST_MSECOND
    );
}

int LocalMediaService::getDurationMs() const {
    if (!s_pipeline) return 0;
    gint64 dur = queryDuration();
    return dur < 0 ? 0 : static_cast<int>(dur / GST_MSECOND);
}

int LocalMediaService::getPositionMs() const {
    if (!s_pipeline) return 0;
    gint64 pos = queryPosition();
    return pos < 0 ? 0 : static_cast<int>(pos / GST_MSECOND);
}

// ── Audio visualizer stubs ────────────────────────────────────────────────────

extern "C" {

int sp_audio_get_pcm(float* /*outBuffer*/, int /*maxCount*/) { return 0; }
int sp_audio_get_fft(float* /*outBuffer*/, int /*maxCount*/) { return 0; }

}
