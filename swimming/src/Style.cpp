#include "swimming/Style.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <unordered_map>
#include <vector>

namespace swimming {
    namespace {
        std::string trim(const std::string& s) {
            size_t begin = 0, end = s.size();
            while (begin < end && std::isspace(static_cast<unsigned char>(s[begin]))) ++begin;
            while (end > begin && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
            return s.substr(begin, end - begin);
        }

        std::string lower(std::string s) {
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return s;
        }

        bool endsWith(const std::string& s, const std::string& suffix) {
            return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
        }

        /// Parses all of s as a number; nullopt if anything is left over.
        std::optional<float> number(const std::string& s) {
            const std::string t = trim(s);
            if (t.empty()) return std::nullopt;
            char* end = nullptr;
            const float v = std::strtof(t.c_str(), &end);
            if (end != t.c_str() + t.size()) return std::nullopt;
            return v;
        }

        /// Splits on sep outside parentheses and quotes.
        std::vector<std::string> split(const std::string& s, char sep) {
            std::vector<std::string> parts;
            std::string current;
            int depth = 0;
            char quote = 0;
            for (char c : s) {
                if (quote) {
                    if (c == quote) quote = 0;
                } else if (c == '"' || c == '\'') {
                    quote = c;
                } else if (c == '(') {
                    ++depth;
                } else if (c == ')') {
                    depth = std::max(0, depth - 1);
                } else if (c == sep && depth == 0) {
                    parts.push_back(current);
                    current.clear();
                    continue;
                }
                current += c;
            }
            parts.push_back(current);
            return parts;
        }

        std::vector<std::string> words(const std::string& s) {
            std::vector<std::string> out;
            std::string current;
            for (char c : s) {
                if (std::isspace(static_cast<unsigned char>(c))) {
                    if (!current.empty()) out.push_back(current);
                    current.clear();
                } else {
                    current += c;
                }
            }
            if (!current.empty()) out.push_back(current);
            return out;
        }

        std::uint8_t clampByte(float v) {
            return static_cast<std::uint8_t>(std::lround(std::clamp(v, 0.0f, 255.0f)));
        }

        const std::unordered_map<std::string, std::uint32_t>& namedColors() {
            static const std::unordered_map<std::string, std::uint32_t> colors = {
                {"aliceblue", 0xf0f8ff}, {"antiquewhite", 0xfaebd7}, {"aqua", 0x00ffff}, {"aquamarine", 0x7fffd4},
                {"azure", 0xf0ffff}, {"beige", 0xf5f5dc}, {"bisque", 0xffe4c4}, {"black", 0x000000},
                {"blanchedalmond", 0xffebcd}, {"blue", 0x0000ff}, {"blueviolet", 0x8a2be2}, {"brown", 0xa52a2a},
                {"burlywood", 0xdeb887}, {"cadetblue", 0x5f9ea0}, {"chartreuse", 0x7fff00}, {"chocolate", 0xd2691e},
                {"coral", 0xff7f50}, {"cornflowerblue", 0x6495ed}, {"cornsilk", 0xfff8dc}, {"crimson", 0xdc143c},
                {"cyan", 0x00ffff}, {"darkblue", 0x00008b}, {"darkcyan", 0x008b8b}, {"darkgoldenrod", 0xb8860b},
                {"darkgray", 0xa9a9a9}, {"darkgreen", 0x006400}, {"darkgrey", 0xa9a9a9}, {"darkkhaki", 0xbdb76b},
                {"darkmagenta", 0x8b008b}, {"darkolivegreen", 0x556b2f}, {"darkorange", 0xff8c00}, {"darkorchid", 0x9932cc},
                {"darkred", 0x8b0000}, {"darksalmon", 0xe9967a}, {"darkseagreen", 0x8fbc8f}, {"darkslateblue", 0x483d8b},
                {"darkslategray", 0x2f4f4f}, {"darkslategrey", 0x2f4f4f}, {"darkturquoise", 0x00ced1}, {"darkviolet", 0x9400d3},
                {"deeppink", 0xff1493}, {"deepskyblue", 0x00bfff}, {"dimgray", 0x696969}, {"dimgrey", 0x696969},
                {"dodgerblue", 0x1e90ff}, {"firebrick", 0xb22222}, {"floralwhite", 0xfffaf0}, {"forestgreen", 0x228b22},
                {"fuchsia", 0xff00ff}, {"gainsboro", 0xdcdcdc}, {"ghostwhite", 0xf8f8ff}, {"gold", 0xffd700},
                {"goldenrod", 0xdaa520}, {"gray", 0x808080}, {"green", 0x008000}, {"greenyellow", 0xadff2f},
                {"grey", 0x808080}, {"honeydew", 0xf0fff0}, {"hotpink", 0xff69b4}, {"indianred", 0xcd5c5c},
                {"indigo", 0x4b0082}, {"ivory", 0xfffff0}, {"khaki", 0xf0e68c}, {"lavender", 0xe6e6fa},
                {"lavenderblush", 0xfff0f5}, {"lawngreen", 0x7cfc00}, {"lemonchiffon", 0xfffacd}, {"lightblue", 0xadd8e6},
                {"lightcoral", 0xf08080}, {"lightcyan", 0xe0ffff}, {"lightgoldenrodyellow", 0xfafad2}, {"lightgray", 0xd3d3d3},
                {"lightgreen", 0x90ee90}, {"lightgrey", 0xd3d3d3}, {"lightpink", 0xffb6c1}, {"lightsalmon", 0xffa07a},
                {"lightseagreen", 0x20b2aa}, {"lightskyblue", 0x87cefa}, {"lightslategray", 0x778899}, {"lightslategrey", 0x778899},
                {"lightsteelblue", 0xb0c4de}, {"lightyellow", 0xffffe0}, {"lime", 0x00ff00}, {"limegreen", 0x32cd32},
                {"linen", 0xfaf0e6}, {"magenta", 0xff00ff}, {"maroon", 0x800000}, {"mediumaquamarine", 0x66cdaa},
                {"mediumblue", 0x0000cd}, {"mediumorchid", 0xba55d3}, {"mediumpurple", 0x9370db}, {"mediumseagreen", 0x3cb371},
                {"mediumslateblue", 0x7b68ee}, {"mediumspringgreen", 0x00fa9a}, {"mediumturquoise", 0x48d1cc}, {"mediumvioletred", 0xc71585},
                {"midnightblue", 0x191970}, {"mintcream", 0xf5fffa}, {"mistyrose", 0xffe4e1}, {"moccasin", 0xffe4b5},
                {"navajowhite", 0xffdead}, {"navy", 0x000080}, {"oldlace", 0xfdf5e6}, {"olive", 0x808000},
                {"olivedrab", 0x6b8e23}, {"orange", 0xffa500}, {"orangered", 0xff4500}, {"orchid", 0xda70d6},
                {"palegoldenrod", 0xeee8aa}, {"palegreen", 0x98fb98}, {"paleturquoise", 0xafeeee}, {"palevioletred", 0xdb7093},
                {"papayawhip", 0xffefd5}, {"peachpuff", 0xffdab9}, {"peru", 0xcd853f}, {"pink", 0xffc0cb},
                {"plum", 0xdda0dd}, {"powderblue", 0xb0e0e6}, {"purple", 0x800080}, {"rebeccapurple", 0x663399},
                {"red", 0xff0000}, {"rosybrown", 0xbc8f8f}, {"royalblue", 0x4169e1}, {"saddlebrown", 0x8b4513},
                {"salmon", 0xfa8072}, {"sandybrown", 0xf4a460}, {"seagreen", 0x2e8b57}, {"seashell", 0xfff5ee},
                {"sienna", 0xa0522d}, {"silver", 0xc0c0c0}, {"skyblue", 0x87ceeb}, {"slateblue", 0x6a5acd},
                {"slategray", 0x708090}, {"slategrey", 0x708090}, {"snow", 0xfffafa}, {"springgreen", 0x00ff7f},
                {"steelblue", 0x4682b4}, {"tan", 0xd2b48c}, {"teal", 0x008080}, {"thistle", 0xd8bfd8},
                {"tomato", 0xff6347}, {"turquoise", 0x40e0d0}, {"violet", 0xee82ee}, {"wheat", 0xf5deb3},
                {"white", 0xffffff}, {"whitesmoke", 0xf5f5f5}, {"yellow", 0xffff00}, {"yellowgreen", 0x9acd32},
            };
            return colors;
        }

        std::optional<Rgba> hexColor(const std::string& hex) {
            for (char c : hex)
                if (!std::isxdigit(static_cast<unsigned char>(c))) return std::nullopt;
            auto digit = [&](size_t i) { return static_cast<std::uint8_t>(std::stoi(hex.substr(i, 1), nullptr, 16) * 17); };
            auto pair = [&](size_t i) { return static_cast<std::uint8_t>(std::stoi(hex.substr(i, 2), nullptr, 16)); };
            switch (hex.size()) {
            case 3: return Rgba{digit(0), digit(1), digit(2), 255};
            case 4: return Rgba{digit(0), digit(1), digit(2), digit(3)};
            case 6: return Rgba{pair(0), pair(2), pair(4), 255};
            case 8: return Rgba{pair(0), pair(2), pair(4), pair(6)};
            default: return std::nullopt;
            }
        }

        /// The arguments of rgb()/hsl(): comma or space separated, with an
        /// optional "/ alpha".
        std::vector<std::string> colorArgs(const std::string& inner) {
            std::string s = inner;
            std::replace(s.begin(), s.end(), ',', ' ');
            std::replace(s.begin(), s.end(), '/', ' ');
            return words(s);
        }

        std::optional<float> alphaArg(const std::vector<std::string>& args, size_t i) {
            if (args.size() <= i) return 1.0f;
            const std::string& a = args[i];
            if (endsWith(a, "%")) {
                auto v = number(a.substr(0, a.size() - 1));
                return v ? std::optional<float>(*v / 100.0f) : std::nullopt;
            }
            return number(a);
        }

        std::optional<Rgba> rgbColor(const std::vector<std::string>& args) {
            if (args.size() != 3 && args.size() != 4) return std::nullopt;
            Rgba out{};
            for (size_t i = 0; i < 3; ++i) {
                const std::string& a = args[i];
                auto v = endsWith(a, "%") ? number(a.substr(0, a.size() - 1)) : number(a);
                if (!v) return std::nullopt;
                out[i] = clampByte(endsWith(a, "%") ? *v * 2.55f : *v);
            }
            auto alpha = alphaArg(args, 3);
            if (!alpha) return std::nullopt;
            out[3] = clampByte(*alpha * 255.0f);
            return out;
        }

        std::optional<Rgba> hslColor(const std::vector<std::string>& args) {
            if (args.size() != 3 && args.size() != 4) return std::nullopt;
            std::string hue = args[0];
            if (endsWith(hue, "deg")) hue = hue.substr(0, hue.size() - 3);
            auto h = number(hue);
            auto pct = [](const std::string& a) { return number(endsWith(a, "%") ? a.substr(0, a.size() - 1) : a); };
            auto s = pct(args[1]);
            auto l = pct(args[2]);
            auto alpha = alphaArg(args, 3);
            if (!h || !s || !l || !alpha) return std::nullopt;
            const float hh = std::fmod(std::fmod(*h, 360.0f) + 360.0f, 360.0f) / 360.0f;
            const float ss = std::clamp(*s / 100.0f, 0.0f, 1.0f);
            const float ll = std::clamp(*l / 100.0f, 0.0f, 1.0f);
            const float q = ll < 0.5f ? ll * (1 + ss) : ll + ss - ll * ss;
            const float p = 2 * ll - q;
            auto channel = [&](float t) {
                if (t < 0) t += 1;
                if (t > 1) t -= 1;
                if (t < 1.0f / 6) return p + (q - p) * 6 * t;
                if (t < 0.5f) return q;
                if (t < 2.0f / 3) return p + (q - p) * (2.0f / 3 - t) * 6;
                return p;
            };
            return Rgba{clampByte(channel(hh + 1.0f / 3) * 255), clampByte(channel(hh) * 255),
                        clampByte(channel(hh - 1.0f / 3) * 255), clampByte(*alpha * 255.0f)};
        }

        std::string formatLen(const Len& len) {
            auto text = [](float v) {
                std::string s = std::to_string(v);
                s.erase(s.find_last_not_of('0') + 1);
                if (!s.empty() && s.back() == '.') s.pop_back();
                return s;
            };
            switch (len.kind) {
            case Len::Px: return text(len.value) + "px";
            case Len::Pct: return text(len.value * 100.0f) + "%";
            default: return "auto";
            }
        }
    }

    Decls parseDecls(const std::string& css) {
        Decls out;
        for (const std::string& declaration : split(css, ';')) {
            const size_t colon = declaration.find(':');
            if (colon == std::string::npos) continue;
            const std::string name = lower(trim(declaration.substr(0, colon)));
            std::string value = trim(declaration.substr(colon + 1));
            const size_t important = lower(value).rfind("!important");
            if (important != std::string::npos) value = trim(value.substr(0, important));
            if (name.empty() || value.empty()) continue;
            out[name] = value;
        }
        return out;
    }

    std::optional<Rgba> parseColor(const std::string& value) {
        const std::string v = lower(trim(value));
        if (v.empty()) return std::nullopt;
        if (v == "transparent") return Rgba{0, 0, 0, 0};
        if (v[0] == '#') return hexColor(v.substr(1));
        const size_t open = v.find('(');
        if (open != std::string::npos && v.back() == ')') {
            const std::string fn = trim(v.substr(0, open));
            const auto args = colorArgs(v.substr(open + 1, v.size() - open - 2));
            if (fn == "rgb" || fn == "rgba") return rgbColor(args);
            if (fn == "hsl" || fn == "hsla") return hslColor(args);
            return std::nullopt;
        }
        const auto& named = namedColors();
        const auto it = named.find(v);
        if (it == named.end()) return std::nullopt;
        return Rgba{static_cast<std::uint8_t>(it->second >> 16), static_cast<std::uint8_t>(it->second >> 8),
                    static_cast<std::uint8_t>(it->second), 255};
    }

    std::optional<Len> parseLen(const std::string& value) {
        const std::string s = lower(trim(value));
        if (s == "auto") return Len{};
        if (endsWith(s, "%")) {
            auto v = number(s.substr(0, s.size() - 1));
            return v ? std::optional<Len>(Len::pct(*v / 100.0f)) : std::nullopt;
        }
        for (const char* unit : {"px", "rem", "em"}) {
            if (endsWith(s, unit)) {
                auto v = number(s.substr(0, s.size() - std::char_traits<char>::length(unit)));
                if (!v) return std::nullopt;
                return Len::px(std::string(unit) == "px" ? *v : *v * 16.0f);
            }
        }
        auto v = number(s);
        return v ? std::optional<Len>(Len::px(*v)) : std::nullopt;
    }

    std::optional<std::array<Len, 4>> expandBox(const std::string& value) {
        std::vector<Len> p;
        for (const std::string& word : words(value))
            if (auto len = parseLen(word)) p.push_back(*len);
        switch (p.size()) {
        case 1: return std::array<Len, 4>{p[0], p[0], p[0], p[0]};
        case 2: return std::array<Len, 4>{p[0], p[1], p[0], p[1]};
        case 3: return std::array<Len, 4>{p[0], p[1], p[2], p[1]};
        case 4: return std::array<Len, 4>{p[0], p[1], p[2], p[3]};
        default: return std::nullopt;
        }
    }

    void merge(Decls& dst, const Decls& src) {
        static const char* const sides[] = {"top", "right", "bottom", "left"};
        for (const auto& [name, value] : src) {
            if (name == "margin" || name == "padding" || name == "border-width") {
                if (auto box = expandBox(value)) {
                    for (int i = 0; i < 4; ++i) {
                        const std::string key = name == "border-width"
                            ? "border-" + std::string(sides[i]) + "-width"
                            : name + "-" + sides[i];
                        dst[key] = formatLen((*box)[i]);
                    }
                    continue;
                }
            }
            dst[name] = value;
        }
    }
}
