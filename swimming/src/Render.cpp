#include "swimming/Render.h"

#include <algorithm>
#include <cmath>

namespace swimming {
    namespace {
        /// Signed distance from (px, py) to the rounded rectangle; negative inside.
        float roundedRectDistance(float px, float py, float x, float y, float w, float h, float r) {
            const float cx = x + w / 2, cy = y + h / 2;
            const float qx = std::abs(px - cx) - (w / 2 - r);
            const float qy = std::abs(py - cy) - (h / 2 - r);
            const float outside = std::hypot(std::max(qx, 0.0f), std::max(qy, 0.0f));
            return outside + std::min(std::max(qx, qy), 0.0f) - r;
        }

        /// Rasterizes the rounded rectangle, coverage(distance) giving each pixel's coverage.
        template <typename Coverage>
        void fillShape(Pixmap& pm, float x, float y, float w, float h, float r, float pad, const Rgba& color, Coverage coverage) {
            const int x0 = std::max(0, static_cast<int>(std::floor(x - pad)));
            const int y0 = std::max(0, static_cast<int>(std::floor(y - pad)));
            const int x1 = std::min(static_cast<int>(pm.width), static_cast<int>(std::ceil(x + w + pad)));
            const int y1 = std::min(static_cast<int>(pm.height), static_cast<int>(std::ceil(y + h + pad)));
            for (int py = y0; py < y1; ++py)
                for (int px = x0; px < x1; ++px) {
                    const float d = roundedRectDistance(px + 0.5f, py + 0.5f, x, y, w, h, r);
                    const float c = coverage(d);
                    if (c > 0) pm.blend(static_cast<unsigned>(px), static_cast<unsigned>(py), color, c);
                }
        }

        void paintBox(Pixmap& pm, const LayoutBox& b, float ox, float oy) {
            const float x = ox + b.x, y = oy + b.y;
            const Computed& c = b.computed;
            if (b.id && b.w > 0 && b.h > 0) {
                const float r = std::max(0.0f, std::min({c.radius, b.w / 2, b.h / 2}));
                if (c.background) {
                    Rgba bg = *c.background;
                    bg[3] = static_cast<std::uint8_t>(bg[3] * c.opacity);
                    fillShape(pm, x, y, b.w, b.h, r, 1, bg, [](float d) { return std::clamp(0.5f - d, 0.0f, 1.0f); });
                }
                if (c.borderWidth > 0) {
                    // The stroke is centred on the box edge.
                    const float half = c.borderWidth / 2;
                    fillShape(pm, x, y, b.w, b.h, r, half + 1, c.borderColor,
                              [half](float d) { return std::clamp(half + 0.5f - std::abs(d), 0.0f, 1.0f); });
                }
            }
            for (const LayoutBox& child : b.children)
                paintBox(pm, child, x, y);
        }
    }

    Pixmap::Pixmap(unsigned w, unsigned h) : width(std::max(1u, w)), height(std::max(1u, h)) {
        data.assign(static_cast<size_t>(width) * height * 4, 255);
    }

    Rgba Pixmap::pixel(unsigned x, unsigned y) const {
        const size_t i = (static_cast<size_t>(y) * width + x) * 4;
        return {data[i], data[i + 1], data[i + 2], data[i + 3]};
    }

    void Pixmap::blend(unsigned x, unsigned y, const Rgba& color, float coverage) {
        if (x >= width || y >= height) return;
        const size_t i = (static_cast<size_t>(y) * width + x) * 4;
        const float sa = color[3] / 255.0f * coverage;
        const float da = data[i + 3] / 255.0f;
        const float oa = sa + da * (1 - sa);
        for (int k = 0; k < 3; ++k) {
            const float out = oa > 0 ? (color[k] * sa + data[i + k] * da * (1 - sa)) / oa : 0;
            data[i + k] = static_cast<std::uint8_t>(std::lround(std::clamp(out, 0.0f, 255.0f)));
        }
        data[i + 3] = static_cast<std::uint8_t>(std::lround(oa * 255));
    }

    Pixmap paint(const LaidOut& l, unsigned width, unsigned height) {
        Pixmap pm(width, height);
        paintBox(pm, l.root, 0, 0);
        return pm;
    }
}
