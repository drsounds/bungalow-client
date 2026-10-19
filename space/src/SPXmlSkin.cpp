#include "SPXmlSkin.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "SPAppManager.h"
#include "SPElement.h"
#include "SPGraphicsContext.h"
#include "SPImage.h"
#include "SPTabBarElement.h"
#include "SPXmlParser.h"

namespace fs = std::filesystem;

namespace space {
    namespace {
        // How far the lightness slider (0..1, neutral at 0.5) shifts skin colours.
        const float kLightnessRange = 0.3f;

        std::string attr(const SPXmlNode& node, const char* key, const std::string& def = {}) {
            auto it = node.attributes.find(key);
            return it == node.attributes.end() ? def : it->second;
        }

        float floatAttr(const SPXmlNode& node, const char* key, float def) {
            auto it = node.attributes.find(key);
            if (it == node.attributes.end()) return def;
            try { return std::stof(it->second); } catch (...) { return def; }
        }

        bool boolAttr(const SPXmlNode& node, const char* key, bool def) {
            auto it = node.attributes.find(key);
            if (it == node.attributes.end()) return def;
            return it->second != "false" && it->second != "none" && it->second != "0";
        }

        std::vector<int> parseInts(const std::string& text) {
            std::vector<int> result;
            std::string token;
            auto flush = [&] {
                if (!token.empty()) {
                    try { result.push_back(std::stoi(token)); } catch (...) {}
                    token.clear();
                }
            };
            for (char c : text) {
                if (c == ',' || c == ';' || c == ' ') flush();
                else token += c;
            }
            flush();
            return result;
        }

        int hexDigit(char c) {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        }

        // #rgb, #rrggbb or #rrggbbaa.
        std::optional<SPColor> parseHex(const std::string& text) {
            if (text.empty() || text[0] != '#') return std::nullopt;
            std::string hex = text.substr(1);
            if (hex.size() == 3)
                hex = { hex[0], hex[0], hex[1], hex[1], hex[2], hex[2] };
            if (hex.size() != 6 && hex.size() != 8) return std::nullopt;
            int v[4] = { 0, 0, 0, 255 };
            for (size_t i = 0; i < hex.size(); i += 2) {
                int hi = hexDigit(hex[i]), lo = hexDigit(hex[i + 1]);
                if (hi < 0 || lo < 0) return std::nullopt;
                v[i / 2] = hi * 16 + lo;
            }
            return SPColor(v[0], v[1], v[2], v[3]);
        }

        // "url(#skin)" → "skin"
        std::string urlReference(const std::string& value) {
            if (value.compare(0, 4, "url(") != 0 || value.back() != ')') return {};
            std::string ref = value.substr(4, value.size() - 5);
            if (!ref.empty() && ref[0] == '#') ref.erase(0, 1);
            return ref;
        }

        // Selector aliases for element types whose stylesheet name differs.
        const std::map<std::string, std::string> kTypeAliases = {
            { "treeview", "sp-sidebar" },
        };
    }

    SPXmlSkin::SPXmlSkin() = default;
    SPXmlSkin::~SPXmlSkin() = default;

    bool SPXmlSkin::load(const std::string& skinXmlPath, const std::string& baseDir) {
        std::ifstream in(fs::u8path(skinXmlPath));
        if (!in.is_open()) {
            std::cerr << "Skin not found: " << skinXmlPath << std::endl;
            return false;
        }
        std::ostringstream ss;
        ss << in.rdbuf();

        SPXmlNode root;
        std::string error;
        if (!parseSPXml(ss.str(), root, &error)) {
            std::cerr << "Invalid skin " << skinXmlPath << ": " << error << std::endl;
            return false;
        }
        if (root.name != "skin") {
            std::cerr << "Invalid skin " << skinXmlPath << ": root element must be <skin>" << std::endl;
            return false;
        }

        for (const SPXmlNode& child : root.children) {
            if (child.textNode) continue;
            if (child.name == "name") {
                for (const SPXmlNode& t : child.children)
                    if (t.textNode) m_name += t.text;
            } else if (child.name == "resources") {
                parseResources(child, baseDir);
            } else if (child.name == "selector") {
                parseSelector(child);
            }
        }
        return true;
    }

    void SPXmlSkin::parseResources(const SPXmlNode& node, const std::string& baseDir) {
        for (const SPXmlNode& child : node.children) {
            if (child.textNode) continue;
            if (child.name == "image") {
                const std::string id = attr(child, "id");
                const std::string src = attr(child, "src");
                if (id.empty() || src.empty()) continue;
                std::string path = SPAppManager::instance().resolve(src);
                if (path.empty())
                    path = (fs::u8path(baseDir) / fs::u8path(src)).u8string();
                ImageResource res;
                try {
                    res.image.reset(new SPImage(path));
                } catch (const std::exception& e) {
                    std::cerr << "Skin image " << src << " failed to load: " << e.what() << std::endl;
                    continue;
                }
                res.tint = boolAttr(child, "tint", true);
                res.tintSaturation = floatAttr(child, "tint-saturation", res.tintSaturation);
                m_images[id] = std::move(res);
            } else if (child.name == "color") {
                const std::string name = attr(child, "name");
                if (name.empty()) continue;
                NamedColor color;
                color.tint = boolAttr(child, "tint", true);
                color.tintSaturation = floatAttr(child, "tint-saturation", color.tintSaturation);
                for (const SPXmlNode& state : child.children) {
                    if (state.textNode || state.name != "state") continue;
                    ColorState cs;
                    cs.background = parseHex(attr(state, "background"));
                    cs.foreground = parseHex(attr(state, "foreground"));
                    color.modes[attr(state, "mode", "light")] = cs;
                }
                m_colors[name] = std::move(color);
            }
        }
    }

    void SPXmlSkin::parseSelector(const SPXmlNode& node) {
        auto rule = std::make_unique<Rule>();
        rule->match = attr(node, "match");
        rule->state = attr(node, "state");
        if (rule->match.empty()) return;

        for (const SPXmlNode& child : node.children) {
            if (child.textNode || child.name != "property") continue;
            auto prop = std::make_unique<Property>();
            prop->name = attr(child, "name");
            prop->value = attr(child, "value");
            prop->imageId = urlReference(prop->value);

            if (!prop->imageId.empty()) {
                auto img = m_images.find(prop->imageId);
                if (img == m_images.end()) {
                    std::cerr << "Skin selector " << rule->match << " references unknown image "
                              << prop->imageId << std::endl;
                    continue;
                }
                SPImage* image = img->second.image.get();
                // slice="x1,y1;x2,y2" — the two corners of the sprite, end exclusive.
                std::vector<int> s = parseInts(attr(child, "slice"));
                int x1 = 0, y1 = 0, x2 = image->width, y2 = image->height;
                if (s.size() == 4) {
                    x1 = std::min(s[0], s[2]); x2 = std::max(s[0], s[2]);
                    y1 = std::min(s[1], s[3]); y2 = std::max(s[1], s[3]);
                }
                x1 = std::clamp(x1, 0, static_cast<int>(image->width));
                x2 = std::clamp(x2, 0, static_cast<int>(image->width));
                y1 = std::clamp(y1, 0, static_cast<int>(image->height));
                y2 = std::clamp(y2, 0, static_cast<int>(image->height));
                if (x2 <= x1 || y2 <= y1) {
                    std::cerr << "Skin selector " << rule->match << " has an empty slice" << std::endl;
                    continue;
                }
                prop->sx = x1; prop->sy = y1; prop->sw = x2 - x1; prop->sh = y2 - y1;
                prop->source.reset(image->sliceImage(x1, y1, prop->sw, prop->sh, 0, 0, 0, 0));

                std::vector<int> n = parseInts(attr(child, "nine-patch"));
                if (n.size() == 4) {
                    prop->left = n[0]; prop->top = n[1]; prop->right = n[2]; prop->bottom = n[3];
                }
            }
            rule->properties.push_back(std::move(prop));
        }
        m_rules.push_back(std::move(rule));
    }

    SPColor SPXmlSkin::tintColor(const SPColor& color, float saturationFloor) const {
        float h, s, l;
        color.toHSL(h, s, l);
        const float sat = std::max(s, saturationFloor) * getSaturation();
        const float light = std::clamp(l + (getLightness() - 0.5f) * kLightnessRange, 0.0f, 1.0f);
        SPColor copy = color;
        return SPColor::fromHSL(getHue(), sat, light, copy.getA());
    }

    void SPXmlSkin::refreshTint() {
        if (m_tintedHue == getHue() && m_tintedSaturation == getSaturation() &&
            m_tintedLightness == getLightness())
            return;
        m_tintedHue = getHue();
        m_tintedSaturation = getSaturation();
        m_tintedLightness = getLightness();

        for (auto& rule : m_rules) {
            for (auto& prop : rule->properties) {
                if (!prop->source) continue;
                const ImageResource& res = m_images.at(prop->imageId);
                const SPImage& src = *prop->source;
                auto* out = new SPImage(src.width, src.height);
                for (int i = 0; i < src.numPixels; ++i) {
                    const pixel& p = src.pixels[i];
                    if (!res.tint || p.a == 0) {
                        out->pixels[i] = p;
                        continue;
                    }
                    SPColor c = tintColor(SPColor(p.r, p.g, p.b, p.a), res.tintSaturation);
                    out->pixels[i] = { static_cast<unsigned char>(c.getR()), static_cast<unsigned char>(c.getG()),
                                       static_cast<unsigned char>(c.getB()), p.a };
                }
                prop->tinted.reset(out);
            }
        }
    }

    std::optional<SPColor> SPXmlSkin::resolveColor(const std::string& value, short scheme, bool foreground) const {
        if (value.empty()) return std::nullopt;
        if (value[0] == '#') return parseHex(value);
        if (value[0] != '$') return std::nullopt;

        std::string name = value.substr(1);
        const size_t dot = name.find('.');
        if (dot != std::string::npos) {
            foreground = name.substr(dot + 1) == "foreground";
            name.erase(dot);
        }
        auto it = m_colors.find(name);
        if (it == m_colors.end() || it->second.modes.empty()) return std::nullopt;

        const NamedColor& color = it->second;
        auto mode = color.modes.find(scheme == SP_COLOR_SCHEME_DARK ? "dark" : "light");
        if (mode == color.modes.end()) mode = color.modes.begin();
        const std::optional<SPColor>& c = foreground ? mode->second.foreground : mode->second.background;
        if (!c) return std::nullopt;
        return color.tint ? tintColor(*c, color.tintSaturation) : *c;
    }

    std::vector<std::string> SPXmlSkin::selectorsFor(SPElement* elm) const {
        const std::string type = elm->getType();
        std::vector<std::string> selectors = { "sp-" + type };
        auto alias = kTypeAliases.find(type);
        if (alias != kTypeAliases.end()) selectors.push_back(alias->second);
        const std::string id = elm->getId();
        if (!id.empty()) selectors.push_back("#" + id);
        return selectors;
    }

    const SPXmlSkin::Rule* SPXmlSkin::findRule(const std::vector<std::string>& selectors,
                                               const std::vector<std::string>& states, bool& matched) const {
        matched = false;
        // Later selectors in the list are more specific (#id beats sp-type).
        for (auto sel = selectors.rbegin(); sel != selectors.rend(); ++sel) {
            for (const std::string& state : states) {
                for (const auto& rule : m_rules) {
                    if (rule->match != *sel) continue;
                    matched = true;
                    if (rule->state == state) return rule.get();
                }
            }
            if (matched) return nullptr;
        }
        return nullptr;
    }

    void SPXmlSkin::drawNinePatch(SPGraphicsContext* g, const Property& p, const sp_rectangle& b) const {
        SPImage* image = p.tinted ? p.tinted.get() : p.source.get();
        if (!image || b.width <= 0 || b.height <= 0) return;

        int l = p.left, r = p.right, t = p.top, bt = p.bottom;
        // Shrink the fixed borders proportionally when the target is too small.
        if (l + r > b.width && l + r > 0) { l = b.width * l / (l + r); r = b.width - l; }
        if (t + bt > b.height && t + bt > 0) { t = b.height * t / (t + bt); bt = b.height - t; }

        const int srcX[4] = { 0, std::min(p.left, p.sw), std::max(p.sw - p.right, 0), p.sw };
        const int srcY[4] = { 0, std::min(p.top, p.sh), std::max(p.sh - p.bottom, 0), p.sh };
        const int dstX[4] = { b.x, b.x + l, b.x + b.width - r, b.x + b.width };
        const int dstY[4] = { b.y, b.y + t, b.y + b.height - bt, b.y + b.height };

        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col) {
                const int sw = srcX[col + 1] - srcX[col], sh = srcY[row + 1] - srcY[row];
                const int dw = dstX[col + 1] - dstX[col], dh = dstY[row + 1] - dstY[row];
                if (sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0) continue;
                g->drawImagePart(image, dstX[col], dstY[row], dw, dh, srcX[col], srcY[row], sw, sh);
            }
        }
    }

    void SPXmlSkin::drawRule(SPGraphicsContext* g, const Rule& rule, short scheme, const sp_rectangle& bounds) {
        refreshTint();
        // Solid backgrounds first so sprites can layer on top of them.
        for (const auto& prop : rule.properties) {
            if (prop->name != "background" || !prop->imageId.empty()) continue;
            if (auto color = resolveColor(prop->value, scheme))
                g->fillRectangle(bounds.x, bounds.y, bounds.width, bounds.height, *color);
        }
        for (const auto& prop : rule.properties) {
            if ((prop->name == "skin" || prop->name == "background") && !prop->imageId.empty())
                drawNinePatch(g, *prop, bounds);
        }
    }

    bool SPXmlSkin::drawElement(SPGraphicsContext* g, SPElement* elm, const string& state, const sp_rectangle& bounds) {
        bool matched = false;
        const std::vector<std::string> states = state.empty()
            ? std::vector<std::string>{ "" }
            : std::vector<std::string>{ state, "" };
        const Rule* rule = findRule(selectorsFor(elm), states, matched);
        if (!matched)
            return SPModernSkin::drawElement(g, elm, state, bounds);
        if (rule)
            drawRule(g, *rule, elm->getColorScheme(), bounds);
        return true;
    }

    void SPXmlSkin::drawTabBar(SPGraphicsContext* g, SPTabBarElement* tabBar, int x, int y, int width, int height) {
        bool matched = false;
        const Rule* rule = findRule(selectorsFor(tabBar), { "" }, matched);
        if (!matched) {
            SPModernSkin::drawTabBar(g, tabBar, x, y, width, height);
            return;
        }
        if (rule)
            drawRule(g, *rule, tabBar->getColorScheme(), { x, y, width, height, 0 });
    }

    void SPXmlSkin::drawTabBarTab(SPGraphicsContext* g, SPTabBarElement* tabBar, SPTab* tab, int index, bool active,
                                  int x, int y, int width, int height) {
        bool matched = false;
        const std::vector<std::string> states = active
            ? std::vector<std::string>{ "selected", "active", "" }
            : std::vector<std::string>{ "" };
        const Rule* rule = findRule({ "sp-tabbar-tab" }, states, matched);
        if (!matched) {
            SPModernSkin::drawTabBarTab(g, tabBar, tab, index, active, x, y, width, height);
            return;
        }
        if (rule)
            drawRule(g, *rule, tabBar->getColorScheme(), { x, y, width, height, 0 });
    }
}
