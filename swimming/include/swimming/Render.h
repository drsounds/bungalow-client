#ifndef SWIMMING_RENDER_H
#define SWIMMING_RENDER_H

#include <cstdint>
#include <string>
#include <vector>

#include "swimming/Layout.h"

/// Software painter for headless rendering: backgrounds, borders and rounded
/// corners. Glyphs are not drawn (text boxes are laid out but left empty);
/// hosts with a graphics context draw text themselves.
namespace swimming {
    /// Straight (not premultiplied) RGBA pixels, row by row.
    struct Pixmap {
        unsigned width = 0;
        unsigned height = 0;
        std::vector<std::uint8_t> data;

        Pixmap(unsigned w, unsigned h);
        Rgba pixel(unsigned x, unsigned y) const;
        /// Blends color over (x, y) with the given coverage (0-1).
        void blend(unsigned x, unsigned y, const Rgba& color, float coverage);
    };

    /// Paints l onto a white width x height pixmap.
    Pixmap paint(const LaidOut& l, unsigned width, unsigned height);

    /// Writes p as a PNG file; throws std::runtime_error on failure.
    void savePng(const Pixmap& p, const std::string& path);
}

#endif
