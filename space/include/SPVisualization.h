#pragma once

#include "SPColor.h"
#include <string>
#include <variant>
#include <vector>

namespace space {

/// A single customization parameter for an SPVisualization.
/// Supports float ranges, booleans, colours, and string enumerations.
struct SPVisualizationParam {
    struct FloatParam { float value; float min; float max; float step = 0.01f; };
    struct BoolParam  { bool  value; };
    struct ColorParam { SPColor value; };
    struct EnumParam  { int value; std::vector<std::string> options; };

    std::string name;   ///< Unique persistence key.
    std::string label;  ///< Human-readable label shown in the parameter panel.

    std::variant<FloatParam, BoolParam, ColorParam, EnumParam> data;

    // ── Convenience factories ─────────────────────────────────────────────────

    static SPVisualizationParam makeFloat(const std::string& name,
                                          const std::string& label,
                                          float value, float min, float max,
                                          float step = 0.01f) {
        return { name, label, FloatParam{ value, min, max, step } };
    }
    static SPVisualizationParam makeBool(const std::string& name,
                                         const std::string& label,
                                         bool value) {
        return { name, label, BoolParam{ value } };
    }
    static SPVisualizationParam makeColor(const std::string& name,
                                          const std::string& label,
                                          SPColor value) {
        return { name, label, ColorParam{ value } };
    }
    static SPVisualizationParam makeEnum(const std::string& name,
                                         const std::string& label,
                                         int value,
                                         std::vector<std::string> options) {
        return { name, label, EnumParam{ value, std::move(options) } };
    }
};

/// Abstract base class for OpenGL music visualizations.
///
/// Subclasses populate `params` in their constructor and implement
/// `renderFrame` to draw one frame into the currently-bound GL context.
///
/// Example subclass skeleton:
/// @code
///   class SpectrumBarsViz : public SPVisualization {
///   public:
///       SpectrumBarsViz() {
///           params.push_back(SPVisualizationParam::makeFloat(
///               "barCount", "Bar count", 32, 4, 128, 1));
///           params.push_back(SPVisualizationParam::makeColor(
///               "color", "Bar colour", SPColor(80, 160, 255, 255)));
///       }
///       std::string getName() const override { return "Spectrum Bars"; }
///       void renderFrame(const float* pcm, int pcmN,
///                        const float* fft, int fftN,
///                        int w, int h) override { /* GL draw calls */ }
///   };
/// @endcode
class SPVisualization {
public:
    /// Customization parameters exposed to the UI.
    /// Subclasses push entries in their constructor.
    std::vector<SPVisualizationParam> params;

    virtual ~SPVisualization() = default;

    /// Display name shown in the visualizer parameter panel.
    virtual std::string getName() const = 0;

    /// Draw one frame into the currently-bound OpenGL context.
    ///
    /// @param pcm      Interleaved mono PCM floats (range −1…1); may be null.
    /// @param pcmCount Number of samples in `pcm`.
    /// @param fft      FFT magnitude array (linear scale, DC…Nyquist); may be null.
    /// @param fftCount Number of bins in `fft`.
    /// @param width    Viewport width in pixels.
    /// @param height   Viewport height in pixels.
    virtual void renderFrame(const float* pcm, int pcmCount,
                             const float* fft, int fftCount,
                             int width, int height) = 0;
};

} // namespace space
