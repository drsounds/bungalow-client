#ifndef SWIMMING_STYLE_H
#define SWIMMING_STYLE_H

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>

/// Inline CSS: declaration parsing, shorthand expansion, colours and lengths.
namespace swimming {
    /// Property name -> value text, e.g. "margin-top" -> "16px".
    using Decls = std::map<std::string, std::string>;
    using Rgba = std::array<std::uint8_t, 4>;

    /// Parses "width: 10px; color: red" into declarations. Names are
    /// lowercased and !important is dropped; later declarations win.
    Decls parseDecls(const std::string& css);

    /// Named colours, #rgb[a], #rrggbb[aa], rgb[a]() and hsl[a](). Returns
    /// nullopt for anything else, e.g. currentcolor.
    std::optional<Rgba> parseColor(const std::string& value);

    struct Len {
        enum Kind { Auto, Px, Pct } kind = Auto;
        float value = 0; ///< pixels, or a fraction (0.5 = 50%) for Pct
        static Len px(float v) { return {Px, v}; }
        static Len pct(float v) { return {Pct, v}; }
        bool operator==(const Len& other) const { return kind == other.kind && value == other.value; }
    };

    /// "auto", "10px", "50%", "1.5em" / "2rem" (16px font), "0" or a bare number.
    std::optional<Len> parseLen(const std::string& value);

    /// Expands 1-4 value box shorthands (margin/padding/border-width) into top, right, bottom, left.
    std::optional<std::array<Len, 4>> expandBox(const std::string& value);

    /// Copies src into dst, later values winning. margin, padding and
    /// border-width are expanded into their longhands first.
    void merge(Decls& dst, const Decls& src);
}

#endif
