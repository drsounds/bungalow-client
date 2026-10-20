/// AVAudioPlayerBridge.mm
/// Thin Objective-C++ wrapper around AVAudioEngine + AVAudioPlayerNode,
/// exposed as plain C functions so that the pure-C++ LocalMediaService
/// can call them.
///
/// In addition to transport control, this module installs a tap on the
/// engine's main mixer node to capture PCM data and compute a real FFT
/// via vDSP (Accelerate).  The latest frame is stored in shared buffers
/// that can be read from the render thread via sp_audio_get_pcm / _fft.

#import <AVFoundation/AVFoundation.h>
#include <Accelerate/Accelerate.h>

#include <atomic>
#include <mutex>
#include <cstring>
#include <cmath>

// ── Audio buffer constants ────────────────────────────────────────────────────

static const int kTapBufferSize = 1024;     // PCM samples per tap callback
static const int kFftBinCount   = kTapBufferSize / 2;  // real FFT output bins
static const int kFftLog2N      = 10;       // log2(1024) = 10

// ── Global engine state ───────────────────────────────────────────────────────

static AVAudioEngine*      g_engine     = nil;
static AVAudioPlayerNode*  g_playerNode = nil;
static AVAudioFile*        g_audioFile  = nil;

// Seek baseline: position = g_seekOffsetSec + elapsed since last play/seek.
static double g_seekOffsetSec = 0.0;

// ── PCM / FFT shared buffers (written on audio thread, read on main thread) ──

static std::mutex g_audioBufMutex;
static float      g_pcmBuf[kTapBufferSize] = {};
static float      g_fftBuf[kFftBinCount]   = {};
static int        g_pcmCount = 0;
static int        g_fftCount = 0;

// ── vDSP FFT setup (initialised once) ────────────────────────────────────────

static FFTSetup g_fftSetup = nullptr;

// ── Forward declarations ──────────────────────────────────────────────────────

static void setupEngineIfNeeded();
extern "C" double sp_audio_get_position();

extern "C" {

/// Returns total duration in seconds, or 0.0 if no file loaded.
double sp_audio_get_duration() {
    if (!g_audioFile) return 0.0;
    return (double)g_audioFile.length
         / g_audioFile.fileFormat.sampleRate;
}
/// Load a local file path (not a URI — caller strips "file://").
/// Returns true on success.
bool sp_audio_load(const char* path) {
    @autoreleasepool {
        if (!path || path[0] == '\0') return false;

        setupEngineIfNeeded();

        [g_playerNode stop];
        g_seekOffsetSec = 0.0;

        NSString* nsPath = [NSString stringWithUTF8String:path];
        NSURL*    url    = [NSURL fileURLWithPath:nsPath];

        NSError* err = nil;
        AVAudioFile* file = [[AVAudioFile alloc] initForReading:url error:&err];
        if (err || !file) {
            NSLog(@"[AVAudioPlayerBridge] load failed: %@",
                  err.localizedDescription);
            g_audioFile = nil;
            return false;
        }

        g_audioFile = file;
        return true;
    }
}

void sp_audio_play() {
    @autoreleasepool {
        if (!g_audioFile || !g_playerNode) return;

        AVAudioFramePosition startFrame =
            (AVAudioFramePosition)(g_seekOffsetSec
                * g_audioFile.fileFormat.sampleRate);
        AVAudioFrameCount remaining =
            (AVAudioFrameCount)(g_audioFile.length - startFrame);
        if (remaining == 0) return;

        [g_playerNode scheduleSegment:g_audioFile
                        startingFrame:startFrame
                           frameCount:remaining
                               atTime:nil
                    completionHandler:nil];
        [g_playerNode play];
    }
}

void sp_audio_pause() {
    if (!g_playerNode) return;
    // Capture position before pausing so sp_audio_get_position stays accurate.
    g_seekOffsetSec = sp_audio_get_position();
    [g_playerNode pause];
}

void sp_audio_stop() {
    if (!g_playerNode) return;
    [g_playerNode stop];
    g_seekOffsetSec = 0.0;
}

/// @param seconds  Seek target in seconds.
void sp_audio_seek(double seconds) {
    @autoreleasepool {
        if (!g_audioFile || !g_playerNode) return;

        double duration = sp_audio_get_duration();
        if (seconds < 0.0)     seconds = 0.0;
        if (seconds > duration) seconds = duration;

        bool wasPlaying = g_playerNode.isPlaying;
        [g_playerNode stop];

        g_seekOffsetSec = seconds;

        if (wasPlaying) {
            AVAudioFramePosition startFrame =
                (AVAudioFramePosition)(seconds
                    * g_audioFile.fileFormat.sampleRate);
            AVAudioFrameCount remaining =
                (AVAudioFrameCount)(g_audioFile.length - startFrame);
            if (remaining > 0) {
                [g_playerNode scheduleSegment:g_audioFile
                                startingFrame:startFrame
                                   frameCount:remaining
                                       atTime:nil
                            completionHandler:nil];
                [g_playerNode play];
            }
        }
    }
}

/// Returns current playback position in seconds.
double sp_audio_get_position() {
    if (!g_playerNode || !g_audioFile) return g_seekOffsetSec;

    if (!g_playerNode.isPlaying) return g_seekOffsetSec;

    AVAudioTime* nodeTime = g_playerNode.lastRenderTime;
    if (!nodeTime) return g_seekOffsetSec;

    AVAudioTime* playerTime = [g_playerNode playerTimeForNodeTime:nodeTime];
    if (!playerTime || !playerTime.isSampleTimeValid) return g_seekOffsetSec;

    double elapsed = (double)playerTime.sampleTime / playerTime.sampleRate;
    return g_seekOffsetSec + elapsed;
}

/// Returns true if the player is currently playing.
bool sp_audio_is_playing() {
    return g_playerNode && g_playerNode.isPlaying;
}

// ── Audio data accessors (called from the render thread) ─────────────────────

/// Copy the latest PCM frame into outBuffer.
/// Returns the number of samples written (0 if no audio data is available).
int sp_audio_get_pcm(float* outBuffer, int maxCount) {
    std::lock_guard<std::mutex> lock(g_audioBufMutex);
    int n = (g_pcmCount < maxCount) ? g_pcmCount : maxCount;
    if (n > 0) memcpy(outBuffer, g_pcmBuf, n * sizeof(float));
    return n;
}

/// Copy the latest FFT magnitude frame into outBuffer.
/// Returns the number of bins written (0 if no audio data is available).
int sp_audio_get_fft(float* outBuffer, int maxCount) {
    std::lock_guard<std::mutex> lock(g_audioBufMutex);
    int n = (g_fftCount < maxCount) ? g_fftCount : maxCount;
    if (n > 0) memcpy(outBuffer, g_fftBuf, n * sizeof(float));
    return n;
}

} // extern "C"

// ── Internal helpers ──────────────────────────────────────────────────────────

/// Initialise the AVAudioEngine + player node + mixer tap.
/// Safe to call multiple times — no-ops if already set up.
static void setupEngineIfNeeded() {
    if (g_engine) return;

    g_engine     = [[AVAudioEngine alloc] init];
    g_playerNode = [[AVAudioPlayerNode alloc] init];

    [g_engine attachNode:g_playerNode];
    [g_engine connect:g_playerNode
                   to:g_engine.mainMixerNode
               format:nil];

    // ── PCM tap on the main mixer output ─────────────────────────────────────
    [g_engine.mainMixerNode
        installTapOnBus:0
             bufferSize:(AVAudioFrameCount)kTapBufferSize
                format:nil
                 block:^(AVAudioPCMBuffer* buffer, AVAudioTime* /*when*/) {

        AVAudioFrameCount frameCount = buffer.frameLength;
        if (frameCount == 0 || !buffer.floatChannelData) return;

        // ── Copy mono PCM (use channel 0; mix if stereo) ─────────────────────
        const float* ch0 = buffer.floatChannelData[0];
        const float* ch1 = (buffer.format.channelCount > 1)
                         ? buffer.floatChannelData[1] : nullptr;

        int n = (int)MIN(frameCount, (AVAudioFrameCount)kTapBufferSize);

        float monoSamples[kTapBufferSize] = {};
        if (ch1) {
            // Average the two channels.
            for (int i = 0; i < n; ++i)
                monoSamples[i] = (ch0[i] + ch1[i]) * 0.5f;
        } else {
            memcpy(monoSamples, ch0, n * sizeof(float));
        }

        // ── Compute real FFT via vDSP ─────────────────────────────────────────
        float fftMagnitudes[kFftBinCount] = {};

        if (g_fftSetup && n == kTapBufferSize) {
            // Apply a Hann window to reduce spectral leakage.
            float windowed[kTapBufferSize];
            float window[kTapBufferSize];
            vDSP_hann_window(window, kTapBufferSize, vDSP_HANN_NORM);
            vDSP_vmul(monoSamples, 1, window, 1, windowed, 1, kTapBufferSize);

            // Pack real signal into split-complex form for vDSP.
            float realPart[kFftBinCount], imagPart[kFftBinCount];
            DSPSplitComplex splitComplex = { realPart, imagPart };
            vDSP_ctoz((const DSPComplex*)windowed, 2, &splitComplex, 1, kFftBinCount);

            // Forward FFT.
            vDSP_fft_zrip(g_fftSetup, &splitComplex, 1, kFftLog2N, FFT_FORWARD);

            // After vDSP_fft_zrip the split-complex packing is non-standard:
            // realp[0] = DC component, imagp[0] = Nyquist component (both real).
            // vDSP_zvabs would mangle them into one sqrt(DC²+Nyquist²) value.
            // Unpack them first so each bin's magnitude is computed correctly.
            float dc      = splitComplex.realp[0];
            float nyquist = splitComplex.imagp[0];
            splitComplex.realp[0] = 0.0f;
            splitComplex.imagp[0] = 0.0f;

            vDSP_zvabs(&splitComplex, 1, fftMagnitudes, 1, kFftBinCount);
            fftMagnitudes[0] = fabsf(dc);  // restore DC in bin 0

            float scale = 1.0f / (float)kTapBufferSize;
            vDSP_vsmul(fftMagnitudes, 1, &scale, fftMagnitudes, 1, kFftBinCount);
        }

        // ── Store results under lock ──────────────────────────────────────────
        {
            std::lock_guard<std::mutex> lock(g_audioBufMutex);
            memcpy(g_pcmBuf, monoSamples, n * sizeof(float));
            g_pcmCount = n;
            memcpy(g_fftBuf, fftMagnitudes, kFftBinCount * sizeof(float));
            g_fftCount = kFftBinCount;
        }
    }];

    NSError* err = nil;
    if (![g_engine startAndReturnError:&err]) {
        NSLog(@"[AVAudioPlayerBridge] engine start failed: %@",
              err.localizedDescription);
    }

    // ── vDSP FFT setup (one-time) ─────────────────────────────────────────────
    if (!g_fftSetup)
        g_fftSetup = vDSP_create_fftsetup(kFftLog2N, FFT_RADIX2);
}

// ── C bridge ─────────────────────────────────────────────────────────────────
